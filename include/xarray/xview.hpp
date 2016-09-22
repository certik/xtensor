#ifndef XVIEW_HPP
#define XVIEW_HPP

#include <utility>
#include <type_traits>
#include <tuple>
#include <algorithm>

#include "xexpression.hpp"
#include "utils.hpp"
#include "xslice.hpp"
#include "xindex.hpp"

namespace qs
{

    /*********************************
     * squeeze count
     *********************************/

    namespace detail
    {
        template <class T, class... S>
        struct squeeze_count_impl
        {
            static constexpr size_t value = squeeze_count_impl<S...>::value + (std::is_integral<T>::value ? 1 : 0);
        };

        template <>
        struct squeeze_count_impl<void>
        {
            static constexpr size_t value = 0;
        };

        template <size_t I, class T, class... S>
        struct squeeze_count_before_impl
        {
            static constexpr size_t value = I ? (squeeze_count_before_impl<I - 1, S...>::value + (std::is_integral<T>::value ? 1 : 0)) : 0;
        };

        template <size_t I>
        struct squeeze_count_before_impl<I, void>
        {
            static constexpr size_t value = 0;
        };
    }

    template <class... S>
    using squeeze_count = detail::squeeze_count_impl<S..., void>;

    template <size_t I, class... S>
    using squeeze_count_before = detail::squeeze_count_before_impl<I, S..., void>;

    /******************************************
     * Views on xexpressions
     ******************************************/

    // Initialize an xview with an xexpression and a list of slices

    // The xview is valid as long as the underlying expression has not
    // been reshaped.

    // If more slices are provided than the dimension of the underlying
    // expression, the behavior is undefined.

    template <class E, class... S>
    class xview : public xexpression<xview<E, S...>>
    {

    public:

        using self_type = xview<E, S...>;

        using value_type = typename E::value_type;
        using reference = typename E::reference;
        using const_reference = typename E::const_reference;
        using pointer = typename E::pointer;
        using const_pointer = typename E::const_pointer;
        using size_type = typename E::size_type;
        using difference_type = typename E::difference_type;

        using shape_type = array_shape<size_type>;
        using closure_type = const self_type&;

        xview(E&& e, S&&... slices) noexcept
            : m_e(e), m_slices(get_xslice_type<S>(slices)...)
        {
        }

        inline size_type dimension() const noexcept
        {
            return m_e.dimension() - squeeze_count<S...>::value;
        }

        //inline bool broadcast_shape(shape_type& shape) const
        //{
        //    auto func = [&shape](bool b, auto&& e) { return b && e.broadcast_shape(shape); };
        //    return accumulate(func, true, m_e);
        //}

        template <class... Args>
        reference operator()(Args... args)
        {
            return access_impl(std::make_index_sequence<sizeof...(Args) + squeeze_count<S...>::value>(), args...);
        }

        template <class... Args>
        const_reference operator()(Args... args) const
        {
            return access_impl(std::make_index_sequence<sizeof...(Args) + squeeze_count<S...>::value>(), args...);
        }

    private:

        E& m_e;
        std::tuple<get_xslice_type<S>...> m_slices;

        template <size_type... I, class... Args>
        reference access_impl(std::index_sequence<I...>, Args... args)
        {
            return m_e(std::get<I>(m_slices)(argument<I - squeeze_count_before<I, S...>::value>(args...))...);
        }

        template <size_type... I, class... Args>
        const_reference access_impl(std::index_sequence<I...>, Args... args) const
        {
            return m_e(std::get<I>(m_slices)(argument<I - squeeze_count_before<I, S...>::value>(args...))...);
        }
    };

    template <class E, class... S>
    xview<E, S...> make_xview(E& e, S&&... slices)
    {
        return xview<E, S...>(std::forward<E>(e), std::forward<S>(slices)...);
    }

}

#endif

