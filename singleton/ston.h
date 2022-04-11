/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef STON_H
#define STON_H

struct Ston {
public:
    static Ston *instance();
    ~Ston();

public:
    int count();

private:
    explicit Ston();
    static Ston *sharedInstance;

    int m_count = 0;
};

#endif /* STON_H */
