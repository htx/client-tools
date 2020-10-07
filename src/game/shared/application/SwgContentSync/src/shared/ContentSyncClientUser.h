// ======================================================================
//
// ContentSyncClientUser.h
// Copyright 2001 Sony Online Entertainment Inc.
// All Rights Reserved.
//
// ======================================================================

#ifndef INCLUDED_ContentSyncClientUser_H
#define INCLUDED_ContentSyncClientUser_H

// ======================================================================

class ApplicationWindow;

#define strcasecmp _stricmp
#include <clientapi.h>
#include <qstring.h>
#include <vector>

// ======================================================================

class ContentSyncClientUser : public ClientUser
{
public:
	typedef std::vector<QString> ErrorList;

	ContentSyncClientUser(ApplicationWindow &applicationWindow);

	void 	OutputError( const char *errBuf ) override;
	void	OutputInfo( char level, const char *data ) override;
	void 	OutputText( const char *data, int length ) override;

	ErrorList::const_iterator errorsBegin() const;
	ErrorList::const_iterator errorsEnd() const;

private:
	ContentSyncClientUser();
	ContentSyncClientUser(const ContentSyncClientUser &);
	ContentSyncClientUser &operator =(const ContentSyncClientUser &);

	ApplicationWindow    &m_applicationWindow;
	bool                  m_error;
	std::vector<QString>  m_errors;
};

// ======================================================================

#endif
