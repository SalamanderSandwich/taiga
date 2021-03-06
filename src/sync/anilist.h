/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "base/json.h"
#include "base/types.h"
#include "sync/service.h"

namespace anime {
class Item;
}

namespace sync {
namespace anilist {

// API documentation:
// https://anilist.gitbooks.io/anilist-apiv2-docs/
// https://anilist.github.io/ApiV2-GraphQL-Docs/

class Service : public sync::Service {
public:
  Service();
  ~Service() {}

  void BuildRequest(Request& request, HttpRequest& http_request);
  void HandleResponse(Response& response, HttpResponse& http_response);
  bool RequestNeedsAuthentication(RequestType request_type) const;

private:
  REQUEST_AND_RESPONSE(AddLibraryEntry);
  REQUEST_AND_RESPONSE(AuthenticateUser);
  REQUEST_AND_RESPONSE(DeleteLibraryEntry);
  REQUEST_AND_RESPONSE(GetLibraryEntries);
  REQUEST_AND_RESPONSE(GetMetadataById);
  REQUEST_AND_RESPONSE(GetSeason);
  REQUEST_AND_RESPONSE(SearchTitle);
  REQUEST_AND_RESPONSE(UpdateLibraryEntry);

  bool RequestSucceeded(Response& response, const HttpResponse& http_response);

  std::wstring BuildLibraryObject(Request& request) const;
  std::wstring BuildRequestBody(const std::string& query, const Json& variables) const;

  int ParseMediaObject(const Json& json) const;
  int ParseMediaListObject(const Json& json) const;
  void ParseMediaTitleObject(const Json& json, anime::Item& anime_item) const;
  void ParseUserObject(const Json& json);

  bool ParseResponseBody(const std::wstring& body, Response& response, Json& json);

  std::string ExpandQuery(const std::string& query) const;
  std::wstring GetMediaFields() const;
  std::wstring GetMediaListFields() const;
};

}  // namespace anilist
}  // namespace sync
