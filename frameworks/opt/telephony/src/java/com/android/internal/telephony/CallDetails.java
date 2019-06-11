/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.android.internal.telephony;

import com.android.internal.telephony.Call.CallType;

import java.util.Map;
import java.util.Map.Entry;
import java.util.HashMap;
import java.lang.StringBuilder;
import android.util.Log;
import android.text.TextUtils;
import android.net.Uri;
import android.os.SystemProperties;

/**
 * CallDetails class takes care of all the additional details like call type
 * and domain needed for IMS calls. This class is not relevant for non-IMS calls
 */
public class CallDetails {

    /*
     * Type of the call based on the media type and the direction of the media.
     */

    public static final int CALL_TYPE_VOICE = 0; /*
                                                  * Voice call-audio in both
                                                  * directions
                                                  */

    public static final int CALL_TYPE_VS_TX = 1; /*
                                                  * Videoshare call-audio in
                                                  * both directions &
                                                  * transmitting video in uplink
                                                  */

    public static final int CALL_TYPE_VS_RX = 2; /*
                                                  * Videoshare call-audio in
                                                  * both directions & receiving
                                                  * video in downlink
                                                  */

    public static final int CALL_TYPE_VT = 3; /*
                                               * Video call-audio & video in
                                               * both directions
                                               */

    public static final int CALL_TYPE_UNKNOWN = 10; /*
                                                     * Unknown Call type, may be
                                                     * used for answering call
                                                     * with same call type as
                                                     * incoming call. This is
                                                     * only for telephony, not
                                                     * meant to be passed to RIL
                                                     */

    public static final int CALL_DOMAIN_UNKNOWN = 0; /*
                                                      * Unknown domain. Sent by
                                                      * RIL when modem has not
                                                      * yet selected a domain
                                                      * for a call
                                                      */
    public static final int CALL_DOMAIN_CS = 1; /*
                                                 * Circuit switched domain
                                                 */
    public static final int CALL_DOMAIN_PS = 2; /*
                                                 * Packet switched domain
                                                 */
    public static final int CALL_DOMAIN_AUTOMATIC = 3; /*
                                                        * Automatic domain. Sent
                                                        * by Android to indicate
                                                        * that the domain for a
                                                        * new call should be
                                                        * selected by modem
                                                        */
    public static final int CALL_DOMAIN_NOT_SET = 4; /*
                                                      * Init value used
                                                      * internally by telephony
                                                      * until domain is set
                                                      */

    public static final boolean SHIP_BUILD = "true".equals(SystemProperties.get("ro.product_ship", "false"));

    public int call_type;
    public int call_domain;
    public boolean call_isMpty;

    private Map<String, String> mExtras;

    public CallDetails() {
        call_isMpty = false;
        call_type = CALL_TYPE_VOICE;
        call_domain = CALL_DOMAIN_NOT_SET;
        mExtras = new HashMap<String, String>();
    }

    public CallDetails(int callType, int callDomain, String[] extraparams) {
        call_isMpty = false;
        call_type = callType;
        call_domain = callDomain;
        mExtras = getMapFromExtras(extraparams);
    }

    public CallDetails(Call.CallType callType) {
        call_isMpty = false;
        mExtras = new HashMap<String, String>();

        if (callType == CallType.CS_CALL_VOICE) {
            call_domain = CallDetails.CALL_DOMAIN_CS;
            call_type = CallDetails.CALL_TYPE_VOICE;
        } else if (callType == CallType.CS_CALL_VIDEO) {
            call_domain = CallDetails.CALL_DOMAIN_CS;
            call_type = CallDetails.CALL_TYPE_VT;
        } else if (callType == CallType.IMS_CALL_VOICE) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VOICE;
        } else if (callType == CallType.IMS_CALL_HDVIDEO) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "hd");
        } else if (callType == CallType.IMS_CALL_QCIFVIDEO) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "qcif");
        } else if (callType == CallType.IMS_CALL_QVGAVIDEO) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "qvga");
        } else if (callType == CallType.IMS_CALL_VIDEO_SHARE_TX) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VS_TX;
        } else if (callType == CallType.IMS_CALL_VIDEO_SHARE_RX) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VS_RX;
        } else if (callType == CallType.IMS_CALL_HDVIDEO_LAND) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "hd_land");
        } else if (callType == CallType.IMS_CALL_HD720VIDEO) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "hd720");
        } else if (callType == CallType.IMS_CALL_CIFVIDEO) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "cif");
        } else if (callType == CallType.IMS_CALL_CIFVIDEO_LAND) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "cif_land");
        } else if (callType == CallType.IMS_CALL_QVGAVIDEO_LAND) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "qvga_land");
        } else if (callType == CallType.IMS_CALL_HD720VIDEO_LAND) {
            call_domain = CallDetails.CALL_DOMAIN_PS;
            call_type = CallDetails.CALL_TYPE_VT;
            mExtras.put("resolution", "hd720_land");
        } else {
            call_domain = CallDetails.CALL_DOMAIN_NOT_SET;
            call_type = CallDetails.CALL_TYPE_VOICE;
        }
    }

    public CallDetails(CallDetails srcCall) {
        if (srcCall != null) {
            call_isMpty = srcCall.call_isMpty;
            call_type = srcCall.call_type;
            call_domain = srcCall.call_domain;
            mExtras = srcCall.mExtras;
        } else {
            call_isMpty = false;
            call_type = CALL_TYPE_VOICE;
            call_domain = CALL_DOMAIN_NOT_SET;
            mExtras = new HashMap<String, String>();
        }
    }

    public void setExtras(String[] extraparams) {
        mExtras = getMapFromExtras(extraparams);
    }

    public void setExtraValue(String key, String value) {
        mExtras.put(key, value);
    }

    public String getExtraValue(String key) {
        return mExtras.get(key);
    }

    public String[] getExtraStrings() {
        return getExtrasFromMap(mExtras);
    }

    public static String[] getExtrasFromMap(Map<String, String> newExtras) {
        String []extras = null;

        if (newExtras == null) {
            return null;
        }

        // TODO: Merge new extras into extras. For now, just serialize and set them
        extras = new String[newExtras.size()];

        if (extras != null) {
            int i = 0;
            for (Entry<String, String> entry : newExtras.entrySet()) {
                extras[i] = "" + entry.getKey() + "=" + entry.getValue();
            }
        }
        return extras;
    }

    public static Map<String, String> getMapFromExtras(String[] extras) {
        return getMapFromExtras(extras, false);
    }

    public static Map<String, String> getMapFromExtras(String[] extras, boolean needDecode) {
        HashMap<String, String> map = new HashMap<String, String>();

        if (extras == null) {
            return map;
        }

        for (String s : extras) {
            int sep_index = s.indexOf('=');
            if (sep_index < 0) {
                continue;
            }

            String key = s.substring(0, sep_index);
            String value = ((sep_index+1) < s.length()) ? s.substring(sep_index+1) : "";

            map.put(key, needDecode ? Uri.decode(value) : value);
        }

        return map;
    }

    public void setExtrasFromMap(Map<String, String> newExtras) {
        if (newExtras == null) {
            return;
        }
        mExtras = newExtras;
    }

    public String getCsvFromExtras() {
        StringBuilder sb = new StringBuilder();

        if (mExtras.isEmpty()) {
            return "";
        }

        for (Map.Entry<String, String> entry : mExtras.entrySet()) {
            sb.append(entry.getKey() + "=" + Uri.encode(entry.getValue()) + "|");
        }

        return sb.substring(0, sb.length()-1); // remove last "|"
    }

    public void setExtrasFromCsv(String newExtras) {
        mExtras = getMapFromExtras(newExtras.split("\\|"), true);
    }

    public void setIsMpty(boolean isMpty) {
        call_isMpty = isMpty;
    }

    public CallType toCallType() {
        boolean isConferenceCall = mExtras.containsKey("participants");
        String resolution = mExtras.get("resolution");

        if (call_domain == CALL_DOMAIN_PS) {
            if (call_type == CALL_TYPE_VOICE) {
                if (isConferenceCall) {
                    return CallType.IMS_CALL_CONFERENCE;
                } else {
                    return CallType.IMS_CALL_VOICE;
                }
            } else if (call_type == CALL_TYPE_VT) {
                if (isConferenceCall) {
                    return CallType.IMS_CALL_CONFERENCE;
                } else if ("qcif".equals(resolution)) {
                    return CallType.IMS_CALL_QCIFVIDEO;
                } else if ("qvga".equals(resolution)) {
                    return CallType.IMS_CALL_QVGAVIDEO;
                } else if ("hd_land".equals(resolution)) {
                    return CallType.IMS_CALL_HDVIDEO_LAND;
                } else if ("hd720".equals(resolution)) {
                    return CallType.IMS_CALL_HD720VIDEO;
                } else if ("cif".equals(resolution)) {
                    return CallType.IMS_CALL_CIFVIDEO;
                } else if ("cif_land".equals(resolution)) {
                    return CallType.IMS_CALL_CIFVIDEO_LAND;
                } else if ("qvga_land".equals(resolution)) {
                    return CallType.IMS_CALL_QVGAVIDEO_LAND;
                } else if ("hd720_land".equals(resolution)) {
                    return CallType.IMS_CALL_HD720VIDEO_LAND;
                } else {
                    return CallType.IMS_CALL_HDVIDEO;
                }
            } else if (call_type == CALL_TYPE_VS_TX) {
                return CallType.IMS_CALL_VIDEO_SHARE_TX;
            } else if (call_type == CALL_TYPE_VS_RX) {
                return CallType.IMS_CALL_VIDEO_SHARE_RX;
            } else  {
                return CallType.NO_CALL;
            }
        } else if (call_domain == CALL_DOMAIN_CS) {
            if (call_type == CALL_TYPE_VOICE) {
                return CallType.CS_CALL_VOICE;
            } else if (call_type == CALL_TYPE_VT) {
                return CallType.CS_CALL_VIDEO;
            } else {
                return CallType.NO_CALL;
            }
        }
        return CallType.NO_CALL;
    }

    /**
     * @return string representation.
     */
    @Override
    public String toString() {
        if (!SHIP_BUILD) {
            return ("type " + call_type
                    + " domain " + call_domain
                    + " isMpty " + call_isMpty
                    + " " + getCsvFromExtras());
        } else {
            return ("type " + call_type
                    + " domain " + call_domain
                    + " isMpty " + call_isMpty
                    + " extras : xxxxxxxxxx");
        }
    }

    public boolean isChanged(CallDetails details) {
        boolean changed = false;

        if (details == null || details == this) {
            return false;
        }

        if (this.call_type != details.call_type || this.call_domain != details.call_domain) {
            return true;
        }

        if (!mExtras.equals(details.mExtras)) {
            changed = true;
        }

        return changed;
    }
}
