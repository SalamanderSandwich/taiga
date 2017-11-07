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

#include "base/format.h"
#include "base/http.h"
#include "base/log.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "library/anime_util.h"
#include "sync/anilist.h"
#include "sync/anilist_util.h"
#include "taiga/settings.h"

namespace sync {
namespace anilist {

Service::Service() {
  host_ = L"graphql.anilist.co";

  id_ = kAniList;
  canonical_name_ = L"anilist";
  name_ = L"AniList";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;
  http_request.method = L"POST";

  if (Settings.GetBool(taiga::kSync_Service_AniList_UseHttps))
    http_request.url.protocol = base::http::Protocol::Https;

  http_request.header[L"Accept"] = L"application/json";
  http_request.header[L"Accept-Charset"] = L"utf-8";
  http_request.header[L"Accept-Encoding"] = L"gzip";
  http_request.header[L"Content-Type"] = L"application/json";

  if (RequestNeedsAuthentication(request.type))
    http_request.header[L"Authorization"] = L"Bearer " + access_token_;

  switch (request.type) {
    BUILD_HTTP_REQUEST(kAddLibraryEntry, AddLibraryEntry);
    BUILD_HTTP_REQUEST(kAuthenticateUser, AuthenticateUser);
    BUILD_HTTP_REQUEST(kDeleteLibraryEntry, DeleteLibraryEntry);
    BUILD_HTTP_REQUEST(kGetLibraryEntries, GetLibraryEntries);
    BUILD_HTTP_REQUEST(kGetMetadataById, GetMetadataById);
    BUILD_HTTP_REQUEST(kGetSeason, GetSeason);
    BUILD_HTTP_REQUEST(kSearchTitle, SearchTitle);
    BUILD_HTTP_REQUEST(kUpdateLibraryEntry, UpdateLibraryEntry);
  }
}

void Service::HandleResponse(Response& response, HttpResponse& http_response) {
  if (RequestSucceeded(response, http_response)) {
    switch (response.type) {
      HANDLE_HTTP_RESPONSE(kAddLibraryEntry, AddLibraryEntry);
      HANDLE_HTTP_RESPONSE(kAuthenticateUser, AuthenticateUser);
      HANDLE_HTTP_RESPONSE(kDeleteLibraryEntry, DeleteLibraryEntry);
      HANDLE_HTTP_RESPONSE(kGetLibraryEntries, GetLibraryEntries);
      HANDLE_HTTP_RESPONSE(kGetMetadataById, GetMetadataById);
      HANDLE_HTTP_RESPONSE(kGetSeason, GetSeason);
      HANDLE_HTTP_RESPONSE(kSearchTitle, SearchTitle);
      HANDLE_HTTP_RESPONSE(kUpdateLibraryEntry, UpdateLibraryEntry);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Request builders

void Service::AuthenticateUser(Request& request, HttpRequest& http_request) {
  // TODO
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  static const auto query{LR"(
query ($userName: String!) {
  MediaListCollection (userName: $userName, type: ANIME) {
    statusLists {
      ...mediaListFragment
    }
    user {
      id
      name
      mediaListOptions {
        scoreFormat
      }
    }
  }
}

fragment mediaListFragment on MediaList {
  id
  status
  score(format: POINT_100)
  progress
  repeat
  notes
  startedAt { year month day }
  completedAt { year month day }
  updatedAt
  media {
    ...mediaFragment
  }
}

fragment mediaFragment on Media {
  {mediaFields}
})"
  };

  const Json json{
    {
      "query", ExpandQuery(query)
    },
    {
      "variables", {
        {"userName", WstrToStr(request.data[canonical_name_ + L"-username"])},
      }
    }
  };

  http_request.body = StrToWstr(json.dump());
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  static const auto query{LR"(
query ($id: Int!) {
  Media (id: $id, type: ANIME) {
    {mediaFields}
  }
})"
  };

  const Json json{
    {
      "query", ExpandQuery(query)
    },
    {
      "variables", {
        {"id", ToInt(request.data[canonical_name_ + L"-id"])},
      }
    }
  };

  http_request.body = StrToWstr(json.dump());
}

void Service::GetSeason(Request& request, HttpRequest& http_request) {
  // TODO
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  static const auto query{LR"(
query ($query: String) {
  Page {
    media(search: $query, type: ANIME) {
      {mediaFields}
    }
  }
})"
  };

  const Json json{
    {
      "query", ExpandQuery(query)
    },
    {
      "variables", {
        {"query", WstrToStr(request.data[L"title"])},
      }
    }
  };

  http_request.body = StrToWstr(json.dump());
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  // TODO
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  // TODO
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  // TODO
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  // TODO
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  const auto& status_lists = root["data"]["MediaListCollection"]["statusLists"];
  for (const auto& status_list : status_lists) {
    for (const auto& value : status_list) {
      ParseMediaListObject(value);
    }
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseMediaObject(root["data"]["Media"]);
}

void Service::GetSeason(Response& response, HttpResponse& http_response) {
  // TODO
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& media : root["data"]["Page"]["media"]) {
    const auto anime_id = ParseMediaObject(media);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  // TODO
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // TODO
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  // TODO
}

////////////////////////////////////////////////////////////////////////////////

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  // TODO
  return false;
}

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  if (http_response.GetStatusCategory() == 200)
    return true;

  // Error
  Json root;
  std::wstring error_description;

  if (JsonParseString(http_response.body, root)) {
    if (root.count("errors")) {
      const auto& errors = root["errors"];
      if (errors.is_array() && !errors.empty()) {
        const auto& error = errors.front();
        error_description = StrToWstr(JsonReadStr(error, "message"));
      }
    }
  }

  if (error_description.empty()) {
    error_description = L"Unknown error (" +
        canonical_name() + L"|" +
        ToWstr(response.type) + L"|" +
        ToWstr(http_response.code) + L")";
  }

  response.data[L"error"] = name() + L" returned an error: " +
                            error_description;
  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Service::BuildLibraryObject(Request& request) const {
  // TODO
  return {};
}

int Service::ParseMediaObject(const Json& json) const {
  const auto anime_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse anime object:\n{}", StrToWstr(json.dump()));
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetTitle(StrToWstr(JsonReadStr(json["title"], "romaji")));
  anime_item.SetEnglishTitle(StrToWstr(JsonReadStr(json["title"], "english")));
  anime_item.SetJapaneseTitle(StrToWstr(JsonReadStr(json["title"], "native")));
  anime_item.SetType(TranslateSeriesTypeFrom(JsonReadStr(json, "format")));
  anime_item.SetSynopsis(DecodeDescription(JsonReadStr(json, "description")));
  anime_item.SetDateStart(TranslateFuzzyDateFrom(json["startDate"]));
  anime_item.SetDateEnd(TranslateFuzzyDateFrom(json["endDate"]));
  anime_item.SetEpisodeCount(JsonReadInt(json, "episodes"));
  anime_item.SetEpisodeLength(JsonReadInt(json, "duration"));
  anime_item.SetImageUrl(StrToWstr(JsonReadStr(json["coverImage"], "large")));
  anime_item.SetScore(JsonReadInt(json, "averageScore"));
  anime_item.SetPopularity(JsonReadInt(json, "popularity"));

  std::vector<std::wstring> genres;
  for (const auto& genre : json["genres"]) {
    if (genre.is_string())
      genres.push_back(StrToWstr(genre));
  }
  anime_item.SetGenres(genres);

  for (const auto& synonym : json["synonyms"]) {
    if (synonym.is_string())
      anime_item.InsertSynonym(StrToWstr(synonym));
  }

  return AnimeDatabase.UpdateItem(anime_item);
}

int Service::ParseMediaListObject(const Json& json) const {
  const auto anime_id = JsonReadInt(json["media"], "id");
  const auto library_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse library entry #{}", library_id);
    return anime::ID_UNKNOWN;
  }

  ParseMediaObject(json["media"]);

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.AddtoUserList();

  anime_item.SetMyId(ToWstr(library_id));
  anime_item.SetMyStatus(TranslateMyStatusFrom(JsonReadStr(json, "status")));
  anime_item.SetMyScore(JsonReadInt(json, "score"));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(json, "progress"));
  anime_item.SetMyRewatchedTimes(JsonReadInt(json, "repeat"));
  anime_item.SetMyNotes(StrToWstr(JsonReadStr(json, "notes")));
  anime_item.SetMyDateStart(TranslateFuzzyDateFrom(json["startedAt"]));
  anime_item.SetMyDateEnd(TranslateFuzzyDateFrom(json["completedAt"]));
  anime_item.SetMyLastUpdated(StrToWstr(JsonReadStr(json, "updatedAt")));

  return AnimeDatabase.UpdateItem(anime_item);
}

bool Service::ParseResponseBody(const std::wstring& body,
                                Response& response, Json& json) {
  if (JsonParseString(body, json))
    return true;

  switch (response.type) {
    case kGetLibraryEntries:
      response.data[L"error"] = L"Could not parse library entries";
      break;
    case kGetMetadataById:
      response.data[L"error"] = L"Could not parse anime object";
      break;
    case kGetSeason:
      response.data[L"error"] = L"Could not parse season data";
      break;
    case kSearchTitle:
      response.data[L"error"] = L"Could not parse search results";
      break;
    case kUpdateLibraryEntry:
      response.data[L"error"] = L"Could not parse library entry";
      break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::string Service::ExpandQuery(std::wstring query) const {
  ReplaceString(query, L"{mediaFields}", GetMediaFields());
  return WstrToStr(query);
}

std::wstring Service::GetMediaFields() const {
  return LR"(id
title { romaji english native }
format
description
startDate { year month day }
endDate { year month day }
episodes
duration
countryOfOrigin
updatedAt
coverImage { large }
genres
synonyms
averageScore
popularity)";
}

}  // namespace anilist
}  // namespace sync