/*******************************************************************************

   Implementation of Jacobian matrix computing methods.

   This file is part of XAD, a comprehensive C++ library for
   automatic differentiation.

   Copyright (C) 2010-2024 Xcelerit Computing Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

#pragma once

#include <XAD/TypeTraits.hpp>
#include <XAD/XAD.hpp>

#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

namespace xad
{
// adj 2d vector
template <typename T>
std::vector<std::vector<T>> computeJacobian(
    const std::vector<AReal<T>> &vec,
    std::function<std::vector<AReal<T>>(std::vector<AReal<T>> &)> foo,
    std::size_t codomain,
    Tape<T> *tape)
{
    auto v(vec);
    codomain = (codomain == 0) ? foo(v).size() : codomain;
    std::vector<std::vector<T>> matrix(codomain, std::vector<T>(v.size(), 0.0));
    computeJacobian(vec, foo, begin(matrix), end(matrix), codomain, tape);
    return matrix;
}

// adj 2d vector !tape & !codomain
template <typename T>
std::vector<std::vector<T>> computeJacobian(
    const std::vector<AReal<T>> &vec,
    std::function<std::vector<AReal<T>>(std::vector<AReal<T>> &)> foo)
{
    return computeJacobian(vec, foo, 0U, Tape<T>::getActive());
}

// adj 2d vector tape & !codomain
template <typename T>
std::vector<std::vector<T>> computeJacobian(
    const std::vector<AReal<T>> &vec,
    std::function<std::vector<AReal<T>>(std::vector<AReal<T>> &)> foo,
    Tape<T> *tape)
{
    return computeJacobian(vec, foo, 0U, tape);
}

// adj 2d vector !tape & codomain
template <typename T>
std::vector<std::vector<T>> computeJacobian(
    const std::vector<AReal<T>> &vec,
    std::function<std::vector<AReal<T>>(std::vector<AReal<T>> &)> foo,
    std::size_t codomain)
{
    return computeJacobian(vec, foo, codomain, Tape<T>::getActive());
}

// adj iterator
template <typename RowIterator, typename T>
void computeJacobian(const std::vector<AReal<T>> &vec,
                     std::function<std::vector<AReal<T>>(std::vector<AReal<T>> &)> foo,
                     RowIterator first, RowIterator last,
                     std::size_t codomain = 0U,
                     Tape<T> *tape = Tape<T>::getActive())
{
    if (static_cast<std::size_t>(std::distance(first->cbegin(), first->cend())) != vec.size())
        throw OutOfRange("Iterator allocated space doesn't equal codomain");
    static_assert(detail::has_begin<typename std::iterator_traits<RowIterator>::value_type>::value,
                  "RowIterator must dereference to a type that implements a begin() method");
    std::unique_ptr<Tape<T>> t;
    if (!tape)
    {
        t = std::unique_ptr<Tape<T>>(new Tape<T>());
        tape = t.get();
    }

    auto v(vec);

    tape->registerInputs(v);
    tape->newRecording();
    auto y = foo(v);
    std::size_t domain = vec.size();
    codomain = (codomain == 0) ? y.size() : codomain;
    if (static_cast<std::size_t>(std::distance(first, last)) != codomain)
        throw OutOfRange("Iterator allocated space doesn't equal codomain");
    if (y.size() != codomain)
        throw OutOfRange("Iterator allocated space doesn't equal codomain");
    tape->registerOutputs(y);

    auto row = first;
    for (std::size_t i = 0; i < codomain; i++, row++)
    {
        auto col = row->begin();
        derivative(y[i]) = 1.0;
        tape->computeAdjoints();
        for (std::size_t j = 0; j < domain; j++, col++) *col = derivative(v[j]);
        tape->clearDerivatives();
    }
}

// fwd 2d vector
template <typename T>
std::vector<std::vector<T>> computeJacobian(
    const std::vector<FReal<T>> &vec,
    std::function<std::vector<FReal<T>>(std::vector<FReal<T>> &)> foo,
    std::size_t codomain = 0U)
{
    auto v(vec);
    codomain = (codomain == 0) ? foo(v).size() : codomain;
    std::vector<std::vector<T>> matrix(codomain, std::vector<T>(v.size(), 0.0));
    computeJacobian(vec, foo, begin(matrix), end(matrix), codomain);
    return matrix;
}

// fwd iterator
template <typename RowIterator, typename T>
void computeJacobian(const std::vector<FReal<T>> &vec,
                     std::function<std::vector<FReal<T>>(std::vector<FReal<T>> &)> foo,
                     RowIterator first, RowIterator last,
                     std::size_t codomain = 0U)
{
    if (static_cast<std::size_t>(std::distance(first->cbegin(), first->cend())) != vec.size())
        throw OutOfRange("Iterator allocated space doesn't equal codomain");
    static_assert(detail::has_begin<typename std::iterator_traits<RowIterator>::value_type>::value,
                  "RowIterator must dereference to a type that implements a begin() method");

    auto v(vec);
    std::size_t domain = vec.size();
    codomain = (codomain == 0) ? foo(v).size() : codomain;

    if (static_cast<std::size_t>(std::distance(first, last)) != codomain)
        throw OutOfRange("Iterator allocated space doesn't equal codomain");

    auto row = first;
    for (std::size_t i = 0; i < domain; i++)
    {
        derivative(v[i]) = 1.0;
        auto y = foo(v);
        if (i == 0) { // only check once; codomain won't change on further iterations
            if (y.size() != codomain)
                throw OutOfRange("Iterator allocated space doesn't equal codomain");
        }
        derivative(v[i]) = 0.0;
        for (std::size_t j = 0; j < codomain; j++)
        {
            row = first;
            std::advance(row, j);
            auto col = row->begin();
            std::advance(col, i);
            *col = derivative(y[j]);
        }
    }
}
}  // namespace xad
