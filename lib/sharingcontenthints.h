#ifndef SHARINGCONTENTHINTS_H
#define SHARINGCONTENTHINTS_H

#include <QString>

class SharingContentHintsPrivate;

// hints and requirements about the shared content.
class SharingContentHints {
public:
    SharingContentHints();
    ~SharingContentHints();
    SharingContentHints(const SharingContentHints &other);
    SharingContentHints &operator=(const SharingContentHints &other);

    // Main mime type, can be generalized as image/* in case there are multiple files without any being
    // especially important. Empty or "*" matches everything.
    void setMimeType(const QString &mimetype);
    QString mimeType() const;

    void setMultipleFiles(bool multiple);
    bool multipleFiles() const;

private:
    SharingContentHintsPrivate *d_ptr;
};

#endif
