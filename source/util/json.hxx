#pragma once

#include <EASTL/string_view.h>

#ifdef NARUHODO_DEBUG
#define SMOL_JSON_DEBUG
#endif
#ifndef _WIN32
#define SMOL_JSON_ABORT ;
#endif
#define SMOL_JSON_LOG_FN NARUHODO_LOG
#define SMOL_JSON_STRING_VIEW eastl::string_view

#include <vendor/smol_json.hxx>
