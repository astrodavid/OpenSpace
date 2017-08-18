/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __OPENSPACE_MODULE_SYNC___HTTPSYNCHRONIZATION___H__
#define __OPENSPACE_MODULE_SYNC___HTTPSYNCHRONIZATION___H__

#include <openspace/util/resourcesynchronization.h>
#include <openspace/documentation/documentation.h>

#include <ghoul/misc/dictionary.h>

namespace openspace {
    
class HttpSynchronization : public ResourceSynchronization {
public:
    HttpSynchronization(const ghoul::Dictionary& dict);
    bool needsSync() override;
    std::shared_ptr<SynchronizationJob> createSynchronizationJob() override;
    static documentation::Documentation Documentation();
};

class HttpSynchronizationJob : public SynchronizationJob {
public:
	HttpSynchronizationJob();
	virtual ~HttpSynchronizationJob();

	void execute() override;
	std::shared_ptr<SynchronizationProduct> product() override;
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_SYNC___HTTPSYNCHRONIZATION___H__