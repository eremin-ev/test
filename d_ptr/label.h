/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef LABEL_H
#define LABEL_H

#include "widget.h"

/*
#define T_DECL_PRIVATE(T) \
    inline T##Private *d_func() \
    { return reinterpret_cast<T##Private *>(d_ptr); } \
    inline const T##Private *d_func() const \
    { return reinterpret_cast<const T##Private *>(d_ptr); } \
    friend class T##Private;
*/

#define T_D(T) T##Private *const d = d_func()
//#define T_Q(T) T *const q = q_func()

struct LabelPrivate;
struct Label : public Widget {
public:
    explicit Label(const std::string &geometry, const std::string &text);
    ~Label();

    void setText(const std::string &text);
    std::string text() const;

private:
    //LabelPrivate *d_func() { return reinterpret_cast<LabelPrivate *>(d_ptr); }
    //const LabelPrivate *d_func() const { return reinterpret_cast<const LabelPrivate *>(d_ptr); }
    LabelPrivate *d_func() { return reinterpret_cast<LabelPrivate *>(d_ptr); }
    const LabelPrivate *d_func() const { return reinterpret_cast<const LabelPrivate *>(d_ptr); }
};

#endif  /* LABEL_H */
