/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*!
 * \file      ExynosCameraSingleton.h
 * \brief     header file for ExynosCameraSingleton
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2015/06/10
 *
 * <b>Revision History: </b>
 * - 2015/06/10 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

#ifndef EXYNOS_CAMERA_SINGLETON_H
#define EXYNOS_CAMERA_SINGLETON_H

using namespace android;

/* Class declaration */
//! ExynosCameraSingleton is template class to create single object
/*!
 * \ingroup ExynosCamera
 */
template <class T> class ExynosCameraSingleton
{
public:
    //! getInstance
    /*!
    \remarks
        to get singleton object, it must call this API.
        that is why constructor is protected. (caller cannot new object)

        The usage.

        ex 1 : this is original. but other class cannot inherit aa.

            class aa : public ExynosCameraSingleton<aa>
            {
            protected:
                friend class ExynosCameraSingleton<aa>;

                aa();
                virtual ~aa();
            }

            aa *obj = aa::getInstance();


        ex 2  : this make that other class can inherit aa.
            class aa
            {
            protected:
                friend class ExynosCameraSingleton<aa>;

                aa();
                virtual ~aa();
            }

            class bb : public aa
            {
            protected:
                friend class ExynosCameraSingleton<bb>;

                bb();
                virtual ~bb();
            }

            aa *obj1 = ExynosCameraSingleton<aa>::getInstance();
            bb *obj2 = ExynosCameraSingleton<bb>::getInstance();

    */
    static T* getInstance(void)
    {
        static T object;
        return &object;
    }

protected:
    ExynosCameraSingleton() {}
    virtual ~ExynosCameraSingleton() {}

private:
    ExynosCameraSingleton(const ExynosCameraSingleton&);
    ExynosCameraSingleton& operator=(const ExynosCameraSingleton&);
};

#endif //EXYNOS_CAMERA_SINGLETON_H
