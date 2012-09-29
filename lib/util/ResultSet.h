// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__RESULT_SET__INCLUDED
#define YB__UTIL__RESULT_SET__INCLUDED

#include <deque>
#include <iterator>
#include <algorithm>
#include <cstddef>
#include <util/Exception.h>

namespace Yb {

template <class RowType>
class ResultSetBase
{
    std::deque<RowType> rows_;
    bool finish_;

    virtual bool fetch(RowType &row) = 0;

    bool ready() const { return rows_.size() > 1; }
    RowType &get_current_row() {
        YB_ASSERT(ready());
        return rows_[1];
    }
    RowType &get_previous_row() {
        YB_ASSERT(!rows_.empty());
        return rows_[0];
    }
    void step_forward() {
        YB_ASSERT(!rows_.empty());
        rows_.pop_front();
    }
    bool fetch_next() {
        if (finish_)
            return false;
        RowType row;
        finish_ = !fetch(row);
        if (!finish_) {
            if (rows_.empty()) {
                RowType row0;
                rows_.push_back(row0);
            }
            rows_.push_back(row);
        }
        return !finish_;
    }
public:
    class iterator;
    friend class iterator;

    ResultSetBase(): finish_(false) {}
    virtual ~ResultSetBase() {}

    class iterator: public std::iterator<std::input_iterator_tag,
            RowType, ptrdiff_t, RowType *, RowType & >
    {
        ResultSetBase &result_set_;
        mutable bool flag_end_;

        iterator();

        void lazy_fetch() const {
            if (!flag_end_ && !result_set_.ready())
                flag_end_ = !result_set_.fetch_next();
        }
    public:
        iterator(ResultSetBase &result_set, bool flag_end)
            : result_set_(result_set)
            , flag_end_(flag_end)
        {}

        RowType &operator *() {
            lazy_fetch();
            YB_ASSERT(!flag_end_);
            return result_set_.get_current_row();
        }

        RowType *operator ->() {
            lazy_fetch();
            YB_ASSERT(!flag_end_);
            return &result_set_.get_current_row();
        }

        RowType *operator ++(int) { // prefix
            lazy_fetch();
            YB_ASSERT(!flag_end_);
            result_set_.step_forward();
            return &result_set_.get_previous_row();
        }

        iterator &operator ++() { // postfix
            lazy_fetch();
            YB_ASSERT(!flag_end_);
            result_set_.step_forward();
            return *this;
        }

        bool operator ==(const iterator &other) const { 
            lazy_fetch();
            other.lazy_fetch();
            return (flag_end_ == other.flag_end_
                    ) && (&result_set_ == &other.result_set_);
        }

        bool operator !=(const iterator &other) const {
            lazy_fetch();
            other.lazy_fetch();
            return (flag_end_ != other.flag_end_
                    ) || (&result_set_ != &other.result_set_);
        }
    };

    typedef iterator const_iterator;

    iterator begin() { return iterator(*this, false); }
    iterator end() { return iterator(*this, true); }
    void load() {
        while (!finish_)
            fetch_next();
    }
};

template<class InputIterator, class Size, class OutputIterator>
inline OutputIterator
    copy_no_more_than_n(InputIterator first, InputIterator last,
                        Size n, OutputIterator result)
{
    for (Size count = 0; first != last &&
             (n < 0? true: count < n); ++first, ++count) {
        *result = *first;
        ++result;
    }
    return result;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__RESULT_SET__INCLUDED
