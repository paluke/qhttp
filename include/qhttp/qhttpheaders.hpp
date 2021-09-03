/** @file qhttpheaders.hpp
 *
 * @copyright (C) 2016
 * @date 2016.06.30
 * @author amir zamani <azadkuh@live.com>
 *
 */

#ifndef __QHTTPHEADERS_HPP__
#define __QHTTPHEADERS_HPP__
///////////////////////////////////////////////////////////////////////////////
#include "qhttpfwd.hpp"
#include <QHash>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
///////////////////////////////////////////////////////////////////////////////

/** A map of request or response headers. */
class Headers : public QHash<QByteArray, QByteArray>
{
public:
    /** checks for a header item, regardless of the case of the characters. */
    bool has(const QByteArray& key) const {
        return contains(key.toLower());
    }

    /** checks if a header has the specified value ignoring the case of the
     * characters. */
    bool keyHasValue(const QByteArray& key, const QByteArray& val) const {
        if ( !contains(key) )
            return false;

        for (const QByteArray &hVal : value(key).split(','))
            if (val.compare(hVal.trimmed(), Qt::CaseInsensitive) == 0)
                return true;
        return false;
    }

    template<class Func>
    void forEach(Func&& f) const {
        qhttp::for_each(constBegin(), constEnd(), f);
    }
};

///////////////////////////////////////////////////////////////////////////////
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // __QHTTPHEADERS_HPP__
