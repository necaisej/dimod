// Copyright 2021 D-Wave Systems Inc.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#pragma once

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace dimod {

/// Encode the domain of a variable.
enum Vartype {
    BINARY,  ///< Variables that are either 0 or 1.
    SPIN,    ///< Variables that are either -1 or 1.
    INTEGER  ///< Variables that are integer valued.
};

/**
 * Used internally by QuadraticModelBase to sparsely encode the neighborhood of 
 * a variable.
 *
 * Internally, Neighborhoods keep two vectors, one of neighbors and the other
 * of biases. Neighborhoods are designed to make access more like a standard
 * library map.
 */
template <class Bias, class Neighbor>
class Neighborhood {
 public:
    /// The first template parameter (Bias).
    using bias_type = Bias;

    /// The type for variable indices
    using index_type = std::ptrdiff_t;

    /// The second template parameter (Neighbor).
    using neighbor_type = Neighbor;

    /// Unsigned integral type that can represent non-negative values.
    using size_type = std::size_t;

 private:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;

        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const neighbor_type&, bias_type&>;
        using pointer = value_type*;
        using reference = value_type;

        using bias_iterator = typename std::vector<bias_type>::iterator;
        using neighbor_iterator =
                typename std::vector<neighbor_type>::const_iterator;

        Iterator(neighbor_iterator nit, bias_iterator bit)
                : neighbor_it_(nit), bias_it_(bit) {}

        Iterator& operator++() {
            bias_it_++;
            neighbor_it_++;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.neighbor_it_ == b.neighbor_it_ && a.bias_it_ == b.bias_it_;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return !(a == b);
        }

        value_type operator*() { return value_type{*neighbor_it_, *bias_it_}; }

     private:
        bias_iterator bias_it_;
        neighbor_iterator neighbor_it_;

        friend class Neighborhood;
    };

    struct ConstIterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const neighbor_type&, const bias_type&>;
        using pointer = value_type*;
        using reference = value_type;

        using bias_iterator = typename std::vector<bias_type>::const_iterator;
        using neighbor_iterator =
                typename std::vector<neighbor_type>::const_iterator;

        ConstIterator() {}

        ConstIterator(neighbor_iterator nit, bias_iterator bit)
                : neighbor_it_(nit), bias_it_(bit) {}

        ConstIterator& operator++() {
            bias_it_++;
            neighbor_it_++;
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const ConstIterator& a,
                               const ConstIterator& b) {
            return a.neighbor_it_ == b.neighbor_it_ && a.bias_it_ == b.bias_it_;
        }

        friend bool operator!=(const ConstIterator& a,
                               const ConstIterator& b) {
            return !(a == b);
        }

        const value_type operator*() const {
            return value_type{*neighbor_it_, *bias_it_};
        }

     private:
        bias_iterator bias_it_;
        neighbor_iterator neighbor_it_;

        friend class Neighborhood;
    };

 public:
    /// A forward iterator to `pair<const neighbor_type&, bias_type&>`.
    using iterator = Iterator;

    /// A forward iterator to `const pair<const neighbor_type&, const
    /// bias_type&>.`
    using const_iterator = ConstIterator;

    /**
     * Return a reference to the bias associated with `v`.
     *
     * This function automatically checks whether `v` is a variable in the
     * neighborhood and throws a `std::out_of_range` exception if it is not.
     */
    bias_type at(index_type v) const {
        auto it = std::lower_bound(neighbors.begin(), neighbors.end(), v);
        size_type idx = std::distance(neighbors.begin(), it);
        if (it != neighbors.end() && (*it) == v) {
            // it exists
            return quadratic_biases[idx];
        } else {
            // it doesn't exist
            throw std::out_of_range("given variables have no interaction");
        }
    }

    /// Returns an iterator to the beginning.
    iterator begin() {
        return iterator{neighbors.cbegin(), quadratic_biases.begin()};
    }

    /// Returns an iterator to the end.
    iterator end() {
        return iterator{neighbors.cend(), quadratic_biases.end()};
    }

    /// Returns a const_iterator to the beginning.
    const_iterator cbegin() const {
        return const_iterator{neighbors.cbegin(), quadratic_biases.cbegin()};
    }

    /// Returns a const_iterator to the end.
    const_iterator cend() const {
        return const_iterator{neighbors.cend(), quadratic_biases.cend()};
    }

    /**
     * Insert a neighbor, bias pair at the end of the neighborhood.
     *
     * Note that this does not keep the neighborhood self-consistent and should
     * only be used when you know that the neighbor is greater than the current
     * last element.
     */
    void emplace_back(index_type v, bias_type bias) {
        neighbors.push_back(v);
        quadratic_biases.push_back(bias);
    }

    /**
     * Erase an element from the neighborhood.
     *
     * Returns the number of element removed, either 0 or 1.
     */
    size_type erase(index_type v) {
        auto it = std::lower_bound(neighbors.begin(), neighbors.end(), v);
        if (it != neighbors.end() && (*it) == v) {
            // is there to erase
            size_type idx = std::distance(neighbors.begin(), it);

            neighbors.erase(it);
            quadratic_biases.erase(quadratic_biases.begin() + idx);

            return 1;
        } else {
            return 0;
        }
    }

    /// Erase elements from the neighborhood.
    void erase(iterator first, iterator last) {
        quadratic_biases.erase(first.bias_it_, last.bias_it_);
        neighbors.erase(first.neighbor_it_, last.neighbor_it_);
    }

    /// Return an iterator to the first element that does not come before `v`.
    iterator lower_bound(index_type v) {
        auto it = std::lower_bound(neighbors.begin(), neighbors.end(), v);
        return iterator{it, quadratic_biases.begin() +
                                    std::distance(neighbors.begin(), it)};
    }

    /**
     * Return the bias at neighbor `v` or the default value.
     *
     * Return the bias of `v` if `v` is in the neighborhood, otherwise return
     * the `value` provided without inserting `v`.
     */
    bias_type get(index_type v, bias_type value = 0) const {
        auto it = std::lower_bound(neighbors.begin(), neighbors.end(), v);
        size_type idx = std::distance(neighbors.begin(), it);
        if (it != neighbors.end() && (*it) == v) {
            // it exists
            return quadratic_biases[idx];
        } else {
            // it doesn't exist
            return value;
        }
    }

    /// Return the size of the neighborhood.
    size_type size() const { return neighbors.size(); }

    /**
     * Access the bias of `v`.
     *
     * If `v` is in the neighborhood, the function returns a reference to its
     * bias. If `v` is not in the neighborhood, it is inserted and a reference
     * is returned to its bias.
     */
    bias_type& operator[](index_type v) {
        auto it = std::lower_bound(neighbors.begin(), neighbors.end(), v);
        size_type idx = std::distance(neighbors.begin(), it);
        if (it == neighbors.end() || (*it) != v) {
            // it doesn't exist so insert
            neighbors.insert(it, v);
            quadratic_biases.insert(quadratic_biases.begin() + idx, 0);
        }

        return quadratic_biases[idx];
    }

 protected:
    std::vector<neighbor_type> neighbors;
    std::vector<bias_type> quadratic_biases;
};

template <class Bias, class Neighbor = std::int64_t>
class QuadraticModelBase {
 public:
    /// The first template parameter (Bias)
    using bias_type = Bias;

    /// The type for variable indices
    using index_type = std::ptrdiff_t;

    /// The second template parameter (Neighbor)
    using neighbor_type = Neighbor;

    /// Unsigned integral type that can represent non-negative values.
    using size_type = std::size_t;

    /// A forward iterator to `pair<const neighbor_type&, bias_type&>`
    using const_neighborhood_iterator =
            typename Neighborhood<bias_type, neighbor_type>::const_iterator;

    QuadraticModelBase() : offset_(0) {}

    /// Return True if the model has no quadratic biases.
    bool is_linear() const {
        for (auto it = adj_.begin(); it != adj_.end(); ++it) {
            if ((*it).size()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Return the energy of the given sample.
     *
     * The `sample_start` must be random access iterator pointing to the
     * beginning of the sample.
     *
     * The behavior of this function is undefined when the sample is not
     * `num_variables()` long.
     */
    template <class Iter>  // todo: allow different return types
    bias_type energy(Iter sample_start) {
        bias_type en = offset();

        for (index_type u = 0; u < num_variables(); ++u) {
            auto u_val = *(sample_start + u);

            en += u_val * linear(u);

            auto span = neighborhood(u);
            for (auto nit = span.first; nit != span.second && (*nit).first < u;
                 ++nit) {
                auto v_val = *(sample_start + (*nit).first);
                auto bias = (*nit).second;
                en += u_val * v_val * bias;
            }
        }

        return en;
    }

    /**
     * Return the energy of the given sample.
     *
     * The `sample` must be a vector containing the sample.
     *
     * The behavior of this function is undefined when the sample is not
     * `num_variables()` long.
     */
    template <class T>
    bias_type energy(const std::vector<T>& sample) {
        // todo: check length?
        return energy(sample.cbegin());
    }

    /// Return a reference to the linear bias associated with `v`.
    bias_type& linear(index_type v) { return linear_biases_[v]; }

    /// Return a reference to the linear bias associated with `v`.
    const bias_type& linear(index_type v) const { return linear_biases_[v]; }

    /// Return a pair of iterators - the start and end of the neighborhood
    std::pair<const_neighborhood_iterator, const_neighborhood_iterator>
    neighborhood(index_type u) const {
        return std::make_pair(adj_[u].cbegin(), adj_[u].cend());
    }

    /**
     * Return the quadratic bias associated with `u`, `v`.
     *
     * If `u` and `v` do not have a quadratic bias, returns 0.
     *
     * Note that this function does not return a reference, this is because
     * each quadratic bias is stored twice.
     *
     */
    bias_type quadratic(index_type u, neighbor_type v) const {
        return adj_[u].get(v);
    }

    /**
     * Return the quadratic bias associated with `u`, `v`.
     *
     * Note that this function does not return a reference, this is because
     * each quadratic bias is stored twice.
     *
     * Raises an `out_of_range` error if either `u` or `v` are not variables or
     * if they do not have an interaction then the function throws an exception.
     */
    bias_type quadratic_at(index_type u, index_type v) const {
        return adj_[u].at(v);
    }

    /// Return the number of variables in the quadratic model.
    size_type num_variables() const { return linear_biases_.size(); }

    /// Return the number of interactions in the quadratic model.
    size_type num_interactions() const {
        size_type count = 0;

        for (auto it = adj_.begin(); it != adj_.end(); ++it) {
            count += (*it).size();
        }

        return count / 2;
    }

    /// The number of other variables `v` interacts with.
    size_type num_interactions(index_type v) const { return adj_[v].size(); }

    /// Return a reference to the offset
    bias_type& offset() { return offset_; }

    /// Return a reference to the offset
    const bias_type& offset() const { return offset_; }

    /// Remove the interaction if it exists
    bool remove_interaction(index_type u, index_type v) {
        if (adj_[u].erase(v)) {
            return adj_[v].erase(u);  // should always be true
        } else {
            return false;
        }
    }

 protected:
    std::vector<bias_type> linear_biases_;
    std::vector<Neighborhood<bias_type, neighbor_type>> adj_;

    bias_type offset_;
};

/**
 * A Binary Quadratic Model is a quadratic polynomial over binary variables.
 *
 * Internally, BQMs are stored in a vector-of-vectors adjacency format.
 *
 */
template <class Bias, class Neighbor = std::int64_t>
class BinaryQuadraticModel : public QuadraticModelBase<Bias, Neighbor> {
 public:
    /// The type of the base class.
    using base_type = QuadraticModelBase<Bias, Neighbor>;

    /// The first template parameter (Bias).
    using bias_type = typename base_type::bias_type;

    /// The type for variable indices
    using index_type = std::ptrdiff_t;

    /// Unsigned integral that can represent non-negative values.
    using size_type = typename base_type::size_type;

    /// The second template parameter (Neighbor)
    using neighbor_type = typename base_type::neighbor_type;

    /// Empty constructor. The vartype defaults to `Vartype::BINARY`.
    BinaryQuadraticModel() : base_type(), vartype_(Vartype::BINARY) {}

    /// Create a BQM of the given `vartype`.
    explicit BinaryQuadraticModel(Vartype vartype)
            : base_type(), vartype_(vartype) {}

    /// Create a BQM with `n` variables of the given `vartype`.
    BinaryQuadraticModel(size_type n, Vartype vartype)
            : BinaryQuadraticModel(vartype) {
        resize(n);
    }

    /**
     * Create a BQM from a dense matrix.
     *
     * `dense` must be an array of length `num_variables^2`.
     *
     * Values on the diagonal are treated differently depending on the variable
     * type.
     * If the BQM is SPIN-valued, then the values on the diagonal are
     * added to the offset.
     * If the BQM is BINARY-valued, then the values on the diagonal are added
     * as linear biases.
     *
     */
    template <class T>
    BinaryQuadraticModel(const T dense[], size_type num_variables,
                         Vartype vartype)
            : BinaryQuadraticModel(num_variables, vartype) {
        add_quadratic(dense, num_variables);
    }

    /// Add quadratic bias for the given variables.
    void add_quadratic(index_type u, index_type v, bias_type bias) {
        if (u == v) {
            if (vartype_ == Vartype::BINARY) {
                base_type::linear(u) += bias;
            } else if (vartype_ == Vartype::SPIN) {
                base_type::offset_ += bias;
            } else {
                throw std::logic_error("unknown vartype");
            }
        } else {
            base_type::adj_[u][v] += bias;
            base_type::adj_[v][u] += bias;
        }
    }

    /*
     * Add quadratic biases to the BQM from a dense matrix.
     *
     * `dense` must be an array of length `num_variables^2`.
     *
     * The behavior of this method is undefined when the bqm has fewer than
     * `num_variables` variables.
     *
     * Values on the diagonal are treated differently depending on the variable
     * type.
     * If the BQM is SPIN-valued, then the values on the diagonal are
     * added to the offset.
     * If the BQM is BINARY-valued, then the values on the diagonal are added
     * as linear biases.
     */
    template <class T>
    void add_quadratic(const T dense[], size_type num_variables) {
        // todo: let users add quadratic off the diagonal with row_offset,
        // col_offset

        assert(num_variables <= base_type::num_variables());

        bool sort_needed = !base_type::is_linear();  // do we need to sort after

        bias_type qbias;
        for (size_type u = 0; u < num_variables; ++u) {
            for (size_type v = u + 1; v < num_variables; ++v) {
                qbias = dense[u * num_variables + v] +
                        dense[v * num_variables + u];

                if (qbias != 0) {
                    base_type::adj_[u].emplace_back(v, qbias);
                    base_type::adj_[v].emplace_back(u, qbias);
                }
            }
        }

        if (sort_needed) {
            throw std::logic_error("not implemented yet");
        }

        // handle the diagonal according to vartype
        if (vartype_ == Vartype::SPIN) {
            // diagonal is added to the offset since -1*-1 == 1*1 == 1
            for (size_type v = 0; v < num_variables; ++v) {
                base_type::offset_ += dense[v * (num_variables + 1)];
            }
        } else if (vartype_ == Vartype::BINARY) {
            // diagonal is added as linear biases since 1*1 == 1, 0*0 == 0
            for (size_type v = 0; v < num_variables; ++v) {
                base_type::linear_biases_[v] += dense[v * (num_variables + 1)];
            }
        } else {
            throw std::logic_error("bad vartype");
        }
    }

    /// Change the vartype of the binary quadratic model
    void change_vartype(Vartype vartype) {
        if (vartype == vartype_) return;  // nothing to do

        bias_type lin_mp, lin_offset_mp, quad_mp, quad_offset_mp, lin_quad_mp;
        if (vartype == Vartype::BINARY) {
            lin_mp = 2;
            lin_offset_mp = -1;
            quad_mp = 4;
            lin_quad_mp = -2;
            quad_offset_mp = .5;
        } else if (vartype == Vartype::SPIN) {
            lin_mp = .5;
            lin_offset_mp = .5;
            quad_mp = .25;
            lin_quad_mp = .25;
            quad_offset_mp = .125;
        } else {
            throw std::logic_error("unexpected vartype");
        }

        for (size_type ui = 0; ui < base_type::num_variables(); ++ui) {
            bias_type lbias = base_type::linear_biases_[ui];

            base_type::linear_biases_[ui] *= lin_mp;
            base_type::offset_ += lin_offset_mp * lbias;

            auto begin = base_type::adj_[ui].begin();
            auto end = base_type::adj_[ui].end();
            for (auto nit = begin; nit != end; ++nit) {
                bias_type qbias = (*nit).second;

                (*nit).second *= quad_mp;
                base_type::linear_biases_[ui] += lin_quad_mp * qbias;
                base_type::offset_ += quad_offset_mp * qbias;
            }
        }

        vartype_ = vartype;
    }

    /// Resize the binary quadratic model to contain n variables.
    void resize(index_type n) {
        if (n < (index_type)base_type::num_variables()) {
            // Clean out any of the to-be-deleted variables from the
            // neighborhoods.
            // This approach is better in the dense case. In the sparse case
            // we could determine which neighborhoods need to be trimmed rather
            // than just doing them all.
            for (index_type v = 0; v < n; ++v) {
                base_type::adj_[v].erase(base_type::adj_[v].lower_bound(n),
                                         base_type::adj_[v].end());
            }
        }

        base_type::linear_biases_.resize(n);
        base_type::adj_.resize(n);
    }

    /// Set the quadratic bias for the given variables.
    void set_quadratic(index_type u, index_type v, bias_type bias) {
        if (u == v) {
            // unlike add_quadratic, this is not really well defined for
            // binary quadratic models. I.e. if there is a linear bias, do we
            // overwrite? Or?
            throw std::domain_error(
                    "Cannot set the quadratic bias of a variable with itself");
        } else {
            base_type::adj_[u][v] = bias;
            base_type::adj_[v][u] = bias;
        }
    }

    /// Return the vartype of the binary quadratic model.
    const Vartype& vartype() const { return vartype_; }

    /// Return the vartype of `v`.
    const Vartype& vartype(size_type v) const { return vartype_; }

 private:
    Vartype vartype_;
};

template <class B, class N>
std::ostream& operator<<(std::ostream& os,
                         const BinaryQuadraticModel<B, N>& bqm) {
    os << "BinaryQuadraticModel\n";

    if (bqm.vartype() == Vartype::SPIN) {
        os << "  vartype: spin\n";
    } else if (bqm.vartype() == Vartype::BINARY) {
        os << "  vartype: binary\n";
    } else {
        os << "  vartype: unkown\n";
    }

    os << "  offset: " << bqm.offset() << "\n";

    os << "  linear (" << bqm.num_variables() << " variables):\n";
    for (size_t v = 0; v < bqm.num_variables(); ++v) {
        auto bias = bqm.linear(v);
        if (bias) {
            os << "    " << v << " " << bias << "\n";
        }
    }

    os << "  quadratic (" << bqm.num_interactions() << " interactions):\n";
    for (size_t u = 0; u < bqm.num_variables(); ++u) {
        auto span = bqm.neighborhood(u);
        for (auto nit = span.first; nit != span.second && (*nit).first < u;
             ++nit) {
            os << "    " << u << " " << (*nit).first << " " << (*nit).second
               << "\n";
        }
    }

    return os;
}

}  // namespace dimod
