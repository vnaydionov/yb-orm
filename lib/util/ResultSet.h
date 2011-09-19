#ifndef YB__ORM__RESULT_SET__INCLUDED
#define YB__ORM__RESULT_SET__INCLUDED

#include <iterator>
#include <algorithm>
#include <util/Exception.h>

namespace Yb {

template <class RowType>
class ResultSetBase
{
    RowType current_row_, previous_row_;

    virtual bool fetch(RowType &row) = 0;
public:
    virtual ~ResultSetBase() {}

    class iterator: public std::iterator<std::input_iterator_tag,
            RowType, ptrdiff_t, const RowType *, const RowType & >
    {
        ResultSetBase &result_set_;
        bool flag_end_;

        iterator();
    public:
        iterator(ResultSetBase &result_set, bool flag_end)
            : result_set_(result_set), flag_end_(flag_end)
        {}

        const RowType &operator *() const {
            YB_ASSERT(!flag_end_);
            return result_set_.get_current_row();
        }

        const RowType *operator ++(int) { // prefix
            YB_ASSERT(!flag_end_);
            flag_end_ = !result_set_.fetch_next();
            return &result_set_.get_previous_row();
        }

        iterator &operator ++() { // postfix
            YB_ASSERT(!flag_end_);
            flag_end_ = !result_set_.fetch_next();
            return *this;
        }

        bool operator ==(const iterator &other) const { 
            return (flag_end_ == other.flag_end_
                    ) && (&result_set_ == &other.result_set_);
        }

        bool operator !=(const iterator &other) const {
            return (flag_end_ != other.flag_end_
                    ) || (&result_set_ != &other.result_set_);
        }
    };

    iterator begin() {
        iterator i(*this, false);
        return ++i;
    }

    iterator end() { return iterator(*this, true); }

    const RowType &get_current_row() const { return current_row_; }
    const RowType &get_previous_row() const { return previous_row_; }

    bool fetch_next() {
        std::swap(previous_row_, current_row_);
        return fetch(current_row_);
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
