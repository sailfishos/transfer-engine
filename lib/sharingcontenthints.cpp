#include "sharingcontenthints.h"

class SharingContentHintsPrivate
{
public:
    QString m_mimetype;
    bool m_multipleFiles = false;
};

SharingContentHints::SharingContentHints()
    : d_ptr(new SharingContentHintsPrivate)
{
}

SharingContentHints::SharingContentHints(const SharingContentHints &other)
    : d_ptr(new SharingContentHintsPrivate)
{
    *d_ptr = *other.d_ptr;
}

SharingContentHints &SharingContentHints::operator=(const SharingContentHints &other)
{
    if (this != &other) {
        *d_ptr = *other.d_ptr;
    }
    return *this;
}

SharingContentHints::~SharingContentHints()
{
    delete d_ptr;
}

void SharingContentHints::setMimeType(const QString &mimetype)
{
    d_ptr->m_mimetype = mimetype;
}

QString SharingContentHints::mimeType() const
{
    return d_ptr->m_mimetype;
}

void SharingContentHints::setMultipleFiles(bool multipleFiles)
{
    d_ptr->m_multipleFiles = multipleFiles;
}

bool SharingContentHints::multipleFiles() const
{
    return d_ptr->m_multipleFiles;
}
