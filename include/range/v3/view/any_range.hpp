// Boost.Range library
//
//  Copyright Eric Niebler 2014.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef RANGES_V3_VIEW_ANY_RANGE_HPP
#define RANGES_V3_VIEW_ANY_RANGE_HPP

#include <memory>
#include <utility>
#include <type_traits>
#include <range/v3/range_fwd.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/range_facade.hpp>

namespace ranges
{
    inline namespace v3
    {
        namespace detail
        {
            template<typename Ref>
            struct any_input_cursor_interface
            {
                virtual ~any_input_cursor_interface() {}
                virtual Ref current() const = 0;
                virtual bool equal(any_input_cursor_interface const &) const = 0;
                virtual void next() = 0;
                virtual any_input_cursor_interface *clone() const = 0;
            };

            template<typename S, typename I>
            struct any_input_sentinel_impl;

            template<typename I>
            struct any_input_cursor_impl
              : any_input_cursor_interface<iterator_reference_t<I>>
            {
            private:
                template<typename S, typename II>
                friend struct any_input_sentinel_impl;
                I it_;
            public:
                any_input_cursor_impl() = default;
                any_input_cursor_impl(I it)
                  : it_(std::move(it))
                {}
                iterator_reference_t<I> current() const override
                {
                    return *it_;
                }
                bool equal(any_input_cursor_interface<iterator_reference_t<I>> const &that) const override
                {
                    any_input_cursor_impl const *pthat =
                        dynamic_cast<any_input_cursor_impl const *>(&that);
                    RANGES_ASSERT(pthat != nullptr);
                    return pthat->it_ == it_;
                }
                void next() override
                {
                    ++it_;
                }
                any_input_cursor_impl *clone() const override
                {
                    return new any_input_cursor_impl{it_};
                }
            };

            template<typename Ref>
            struct any_input_sentinel_interface
            {
                virtual ~any_input_sentinel_interface() {}
                virtual bool equal(any_input_cursor_interface<Ref> const &) const = 0;
                virtual any_input_sentinel_interface *clone() const = 0;
            };

            template<typename S, typename I>
            struct any_input_sentinel_impl
              : any_input_sentinel_interface<iterator_reference_t<I>>
            {
            private:
                S s_;
            public:
                any_input_sentinel_impl() = default;
                any_input_sentinel_impl(S s)
                  : s_(std::move(s))
                {}
                bool equal(any_input_cursor_interface<iterator_reference_t<I>> const &that) const override
                {
                    any_input_cursor_impl<I> const *pthat =
                        dynamic_cast<any_input_cursor_impl<I> const *>(&that);
                    RANGES_ASSERT(pthat != nullptr);
                    return s_ == pthat->it_;
                }
                any_input_sentinel_impl *clone() const override
                {
                    return new any_input_sentinel_impl{s_};
                }
            };

            template<typename Ref>
            struct any_input_sentinel;

            template<typename Ref>
            struct any_input_cursor
            {
            private:
                friend struct any_input_sentinel<Ref>;
                std::unique_ptr<any_input_cursor_interface<Ref>> ptr_;
            public:
                using single_pass = std::true_type;
                any_input_cursor() = default;
                template<typename I,
                    CONCEPT_REQUIRES_(InputIterator<I>() &&
                                      Same<Ref, iterator_reference_t<I>>())>
                any_input_cursor(I it)
                  : ptr_{new any_input_cursor_impl<I>{std::move(it)}}
                {}
                any_input_cursor(any_input_cursor &&) = default;
                any_input_cursor(any_input_cursor const &that)
                  : ptr_{that.ptr_->clone()}
                {}
                any_input_cursor &operator=(any_input_cursor &&) = default;
                any_input_cursor &operator=(any_input_cursor const &that)
                {
                    ptr_.reset(that.ptr_->clone());
                    return *this;
                }
                Ref current() const
                {
                    return ptr_->current();
                }
                bool equal(any_input_cursor const &that) const
                {
                    return ptr_->equal(*that.ptr_);
                }
                void next()
                {
                    ptr_->next();
                }
            };

            template<typename Ref>
            struct any_input_sentinel
            {
            private:
                std::unique_ptr<any_input_sentinel_interface<Ref>> ptr_;
            public:
                any_input_sentinel() = default;
                template<typename I, typename S,
                    CONCEPT_REQUIRES_(Sentinel<S, I>() &&
                                      Same<Ref, iterator_reference_t<I>>())>
                any_input_sentinel(S s, I)
                  : ptr_{new any_input_sentinel_impl<S, I>{std::move(s)}}
                {}
                any_input_sentinel(any_input_sentinel &&) = default;
                any_input_sentinel(any_input_sentinel const &that)
                  : ptr_{that.ptr_->clone()}
                {}
                any_input_sentinel &operator=(any_input_sentinel &&) = default;
                any_input_sentinel &operator=(any_input_sentinel const &that)
                {
                    ptr_.reset(that.ptr_->clone());
                    return *this;
                }
                bool equal(any_input_cursor<Ref> const &that) const
                {
                    return ptr_->equal(*that.ptr_);
                }
                void next()
                {
                    ptr_->next();
                }
            };

            template<typename Ref>
            struct any_input_range_interface
            {
                virtual ~any_input_range_interface() {}
                virtual any_input_cursor<Ref> begin_cursor() const = 0;
                virtual any_input_sentinel<Ref> end_cursor() const = 0;
                virtual any_input_range_interface *clone() const = 0;
            };

            template<typename Rng>
            struct any_input_range_impl
              : any_input_range_interface<range_reference_t<Rng>>
            {
            private:
                detail::base_iterable_holder<Rng> rng_;
            public:
                any_input_range_impl() = default;
                any_input_range_impl(Rng && rng)
                  : rng_{std::forward<Rng>(rng)}
                {}
                any_input_cursor<range_reference_t<Rng>> begin_cursor() const override
                {
                    return {begin(rng_.get())};
                }
                any_input_sentinel<range_reference_t<Rng>> end_cursor() const override
                {
                    return {end(rng_.get()), begin(rng_.get())};
                }
                any_input_range_impl *clone() const override
                {
                    return new any_input_range_impl<Rng>{rng_.get()};
                }
            };
        }

        /// \brief A type-erased InputIterable
        template<typename Ref, bool Inf = false>
        struct any_input_range
          : range_facade<any_input_range<Ref>, Inf>
        {
        private:
            friend range_core_access;
            std::unique_ptr<detail::any_input_range_interface<Ref>> ptr_;
            detail::any_input_cursor<Ref> begin_cursor() const
            {
                return ptr_->begin_cursor();
            }
            detail::any_input_sentinel<Ref> end_cursor() const
            {
                return ptr_->end_cursor();
            }
        public:
            any_input_range() = default;
            template<typename Rng,
                CONCEPT_REQUIRES_(InputIterable<Rng>() &&
                                  Same<Ref, range_reference_t<Rng>>())>
            any_input_range(Rng && rng)
              : ptr_{new detail::any_input_range_impl<Rng>{std::forward<Rng>(rng)}}
            {}
            any_input_range(any_input_range &&) = default;
            any_input_range(any_input_range const &that)
              : ptr_{that.ptr_->clone()}
            {}
            any_input_range &operator=(any_input_range &&) = default;
            any_input_range &operator=(any_input_range const &that)
            {
                ptr_.reset(that.ptr_->clone());
                return *this;
            }
        };
    }
}

#endif
