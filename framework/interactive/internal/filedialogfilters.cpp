/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "filedialogfilters.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <sstream>
#include <vector>

#include "global/stringutils.h"

using namespace muse::interactive;

namespace {
struct NameFilterParts {
    std::string description;
    std::string globs;
};

std::optional<NameFilterParts> splitNameFilter(const std::string& filter)
{
    const bool endsWithGroup = !filter.empty() && filter.back() == ')';
    const size_t open = endsWithGroup ? filter.rfind('(') : std::string::npos;

    if (open == std::string::npos) {
        return std::nullopt;
    }

    return NameFilterParts { filter.substr(0, open), filter.substr(open + 1, filter.size() - open - 2) };
}

std::vector<std::string> splitGlobs(const std::string& globs)
{
    std::istringstream in(globs);
    return { std::istream_iterator<std::string>(in), std::istream_iterator<std::string>() };
}

std::vector<std::string> tokenizeGlob(const std::string& glob)
{
    std::vector<std::string> tokens;

    for (size_t pos = 0; pos < glob.size();) {
        const size_t close = glob[pos] == '[' ? glob.find(']', pos) : std::string::npos;
        const size_t length = close == std::string::npos ? 1 : close - pos + 1;
        tokens.push_back(glob.substr(pos, length));
        pos += length;
    }

    return tokens;
}

bool isPlainLetter(const std::string& token)
{
    return token.size() == 1 && std::isalpha(static_cast<unsigned char>(token.front()));
}

std::string bothCases(const std::string& letter)
{
    const unsigned char c = static_cast<unsigned char>(letter.front());
    return { '[', static_cast<char>(std::tolower(c)), static_cast<char>(std::toupper(c)), ']' };
}

bool needsCaseInsensitiveRewrite(const std::string& glob)
{
    const std::vector<std::string> tokens = tokenizeGlob(glob);
    return std::any_of(tokens.begin(), tokens.end(), isPlainLetter);
}

std::string caseInsensitiveGlobIfNeeded(const std::string& glob)
{
    return needsCaseInsensitiveRewrite(glob) ? caseInsensitiveGlob(glob) : glob;
}
}

std::string muse::interactive::caseInsensitiveGlob(const std::string& glob)
{
    std::string result;
    for (const std::string& token : tokenizeGlob(glob)) {
        result += isPlainLetter(token) ? bothCases(token) : token;
    }
    return result;
}

std::string muse::interactive::caseInsensitiveNameFilter(const std::string& filter)
{
    const std::optional<NameFilterParts> parts = splitNameFilter(filter);
    if (!parts) {
        return filter;
    }

    std::vector<std::string> globs = splitGlobs(parts->globs);
    std::transform(globs.begin(), globs.end(), globs.begin(), caseInsensitiveGlobIfNeeded);

    return parts->description + '(' + muse::strings::join(globs, " ") + ')';
}
