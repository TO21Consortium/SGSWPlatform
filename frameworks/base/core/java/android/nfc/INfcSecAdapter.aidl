/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.nfc;

import android.os.Bundle;


/**
 * {@hide}
 */
interface INfcSecAdapter
{
/* START [P1604040001] - Support Dual-SIM solution */
    void setPreferredSimSlot(int preferedSimSlot);
/* END [P1604040001] - Support Dual-SIM solution */

/* START [P160421001] - Patch for Dynamic SE Selection */
    void clearListenModeRouting(int tech, int proto);
    void setListenModeRouting(int type, int value, int route, int power);
    void commitListenModeRoutingOnly();
    void commitListenModeRoutingAndReset();
/* END [P160421001] - Patch for Dynamic SE Selection */

/* START [16052901F] - Change listen tech mask values */
    void changeListenTechMask(int listen_tech);
/* END [16052901F] - Change listen tech mask values */
}
