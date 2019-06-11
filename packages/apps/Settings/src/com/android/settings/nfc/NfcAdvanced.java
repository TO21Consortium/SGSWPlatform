/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.settings.nfc;

import android.app.ActionBar;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.os.UserManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Switch;

import android.content.BroadcastReceiver;
import com.android.settings.SettingsActivity;
import com.android.settings.widget.SwitchBar;
import com.android.settings.R;
/* START [P1604040001] - Support Dual-SIM solution */
import com.android.secnfc.NfcSecAdapter;
/* END [P1604040001] - Support Dual-SIM solution */

import android.content.SharedPreferences;

public class NfcAdvanced extends Fragment
    implements SwitchBar.OnSwitchChangeListener {
    static final String TAG = "NfcAdvanced";

/* START [P160421001] - Patch for Dynamic SE Selection */
    static final String PREF_NFC_ADVANCED = "NfcAdvancedSettingPrefs";
    static final String PREF_SELECTED_DEFAULT_SE = "selected_default_se";
    static final String PREF_SELECTED_DEFAULT_UICC = "selected_default_uicc";
/* END [P160421001] - Patch for Dynamic SE Selection */

//====================================================
    static final int techA = 0x01;
    static final int techB = 0x02;
    static final int techF = 0x04;
    static final int techAactive = 0x40;
    static final int techBactive = 0x80;

    static final int protocol_ISODEP = 0x01;

    static final int tech = 0x01;
    static final int proto = 0x02;

    static int route = 0;
    static int power = 0x3F;
    static int techValue = techA | techB;
    static int protoValue = protocol_ISODEP;
//====================================================

    private View mView;
    private NfcAdapter mNfcAdapter;

/* START [P1604040001] - Support Dual-SIM solution */
    private NfcSecAdapter mNfcSecAdapter;
/* END [P1604040001] - Support Dual-SIM solution */

    private SwitchBar mSwitchBar;
    private CharSequence mOldActivityTitle;

    private RadioGroup mRadioGroupSE = null;
    private RadioGroup mRadioGroupUICC = null;
    private RadioGroup mRadioGroupPAY = null;
    private RadioGroup mRadioGroupRF = null;

    private RadioButton mRadioButtonHce = null;
    private RadioButton mRadioButtonUicc = null;
    private RadioButton mRadioButtonEse = null;
    private RadioButton mRadioButtonUiccSel1 = null;
    private RadioButton mRadioButtonUiccSel2 = null;
    private RadioButton mRadioButtonPayReady = null;
    private RadioButton mRadioButtonPayExec = null;
    private RadioButton mRadioButtonRfTechA = null;
    private RadioButton mRadioButtonRfTechB = null;
    private RadioButton mRadioButtonRfTechAB = null;
    private RadioButton mRadioButtonRfTechF = null;
    private RadioButton mRadioButtonRfTechAll = null;
    private RadioButton mRadioButtonRfTechNull = null;

    private Context mContext = null;
    private IntentFilter mIntentFilter = null;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (NfcAdapter.ACTION_ADAPTER_STATE_CHANGED.equals(action)) {
                handleNfcStateChanged(intent.getIntExtra(NfcAdapter.EXTRA_ADAPTER_STATE,
                    NfcAdapter.STATE_OFF));
            }
        }
    };

    private SharedPreferences mPrefs = null;
    private SharedPreferences.Editor mPrefsEditor = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final ActionBar actionBar = getActivity().getActionBar();

        mOldActivityTitle = actionBar.getTitle();
        actionBar.setTitle(R.string.nfc_advanced_settings_title);

        mContext = getActivity().getApplicationContext();
        mNfcAdapter = NfcAdapter.getDefaultAdapter(mContext);
/* START [P1604040001] - Support Dual-SIM solution */
        mNfcSecAdapter = NfcSecAdapter.get(mNfcAdapter);
/* END [P1604040001] - Support Dual-SIM solution */
        mIntentFilter = new IntentFilter(NfcAdapter.ACTION_ADAPTER_STATE_CHANGED);

        mPrefs = mContext.getSharedPreferences(PREF_NFC_ADVANCED, Context.MODE_PRIVATE);
        mPrefsEditor = mPrefs.edit();
    }

    @Override
    public void onResume() {
        super.onResume();
        mContext.registerReceiver(mReceiver, mIntentFilter);
    }

    @Override
    public void onPause() {
        mContext.unregisterReceiver(mReceiver);
        super.onPause();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        mView = inflater.inflate(R.layout.nfc_advanced, container, false);

        mRadioButtonHce = (RadioButton) mView.findViewById(R.id.rdbtn_se_hce);
        mRadioButtonUicc = (RadioButton) mView.findViewById(R.id.rdbtn_se_uicc);
        mRadioButtonEse = (RadioButton) mView.findViewById(R.id.rdbtn_se_ese);

        mRadioGroupSE = (RadioGroup) mView.findViewById(R.id.radiogroup_seselect);

        mRadioButtonUiccSel1 = (RadioButton) mView.findViewById(R.id.rdbtn_uicc_sel1);
        mRadioButtonUiccSel2 = (RadioButton) mView.findViewById(R.id.rdbtn_uicc_sel2);

        mRadioGroupUICC = (RadioGroup) mView.findViewById(R.id.radiogroup_uiccselect);

        mRadioButtonPayReady = (RadioButton) mView.findViewById(R.id.rdbtn_pay_ready);
        mRadioButtonPayExec = (RadioButton) mView.findViewById(R.id.rdbtn_pay_exec);

        mRadioGroupPAY = (RadioGroup) mView.findViewById(R.id.radiogroup_payselect);

        mRadioButtonRfTechA = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techa);
        mRadioButtonRfTechB = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techb);
        mRadioButtonRfTechAB = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techab);
        mRadioButtonRfTechF = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techf);
        mRadioButtonRfTechAll = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techall);
        mRadioButtonRfTechNull = (RadioButton) mView.findViewById(R.id.rdbtn_rf_technull);

        mRadioGroupRF = (RadioGroup) mView.findViewById(R.id.radiogroup_rfselect);

        // Get Current Selected SE
        int storedRoute = mPrefs.getInt(PREF_SELECTED_DEFAULT_SE, -1);
        int storedUicc = mPrefs.getInt(PREF_SELECTED_DEFAULT_UICC, -1);

        // Display Current Selected SE
        Log.d(TAG, "storedRoute = " + storedRoute);
        switch (storedRoute) {
            case 0: // HCE
                mRadioGroupSE.check(R.id.rdbtn_se_hce);
                break;
            case 1: // eSE
                mRadioGroupSE.check(R.id.rdbtn_se_ese);
                break;
            case 2: // UICC
                mRadioGroupSE.check(R.id.rdbtn_se_uicc);
                break;
            default:
                mRadioGroupSE.clearCheck();
                break;
        }

        // Display Current Selected UICC Slot
        Log.d(TAG, "storedUicc = " + storedUicc);
        switch (storedUicc) {
            case 1: // Slot #1
                mRadioGroupUICC.check(R.id.rdbtn_uicc_sel1);
                break;
            case 2: // Slot #2
                mRadioGroupUICC.check(R.id.rdbtn_uicc_sel2);
                break;
            default:
                mRadioGroupUICC.clearCheck();
                break;
        }

/* START [P160421001] - Patch for Dynamic SE Selection */
        // Set Listener to SE Selection RadioGroup
        mRadioGroupSE.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if(checkedId==R.id.rdbtn_se_hce) {
                    route = 0x00;
                    mRadioButtonUicc.setEnabled(false);
                    mRadioButtonEse.setEnabled(false);
                    //TODO : Put HCE SE Selection API here
                    mRadioButtonUicc.setEnabled(true);
                    mRadioButtonEse.setEnabled(true);

                    Log.d(TAG, "Default SE Selection HCE  = " + route);
                    mPrefsEditor.putInt(PREF_SELECTED_DEFAULT_SE, route);
                    mPrefsEditor.apply();

                    if(mNfcSecAdapter.setSecNfcDefaultRoute(route) == false)
                        mNfcSecAdapter.setSecNfcDefaultRoute(route);
                }
                else if(checkedId==R.id.rdbtn_se_uicc) {
                    route = 0x02;
                    mRadioButtonHce.setEnabled(false);
                    mRadioButtonEse.setEnabled(false);
                    //TODO : Put UICC SE Selection API here
                    mRadioButtonHce.setEnabled(true);
                    mRadioButtonEse.setEnabled(true);

                    Log.d(TAG, "Default SE Selection UICC = " + route);
                    mPrefsEditor.putInt(PREF_SELECTED_DEFAULT_SE, route);
                    mPrefsEditor.apply();

                    if(mNfcSecAdapter.setSecNfcDefaultRoute(route) == false)
                        mNfcSecAdapter.setSecNfcDefaultRoute(route);
                }
                else if(checkedId==R.id.rdbtn_se_ese) {
                    route = 0x01;
                    mRadioButtonHce.setEnabled(false);
                    mRadioButtonUicc.setEnabled(false);
                    //TODO : Put Embedded SE Selection API here
                    mRadioButtonHce.setEnabled(true);
                    mRadioButtonUicc.setEnabled(true);

                    Log.d(TAG, "Default SE Selection ESE  = " + route);
                    mPrefsEditor.putInt(PREF_SELECTED_DEFAULT_SE, route);
                    mPrefsEditor.apply();

                    if(mNfcSecAdapter.setSecNfcDefaultRoute(route) == false)
                        mNfcSecAdapter.setSecNfcDefaultRoute(route);
                }
            }
        });
/* END [P160421001] - Patch for Dynamic SE Selection */

/* START [P1604040001] - Support Dual-SIM solution */
        // Set Listener to UICC Selection RadioGroup
        mRadioGroupUICC.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if(checkedId==R.id.rdbtn_uicc_sel1) {

                    Log.d(TAG, "Select SIM Slot  = " + 0x01);
                    mPrefsEditor.putInt(PREF_SELECTED_DEFAULT_UICC, 0x01);
                    mPrefsEditor.apply();

                    //TODO : Put UICC#1 Selection API here
                    Log.d(TAG, "Select 1st UICC slot");
                    if(mNfcSecAdapter.setSecNfcPreferredSimSlot(0x01) == false)
                        mNfcSecAdapter.setSecNfcPreferredSimSlot(0x01);

                }
                else if(checkedId==R.id.rdbtn_uicc_sel2) {

                    Log.d(TAG, "Select SIM Slot  = " + 0x02);
                    mPrefsEditor.putInt(PREF_SELECTED_DEFAULT_UICC, 0x02);
                    mPrefsEditor.apply();

                    //TODO : Put UICC#2 Selection API here
                    Log.d(TAG, "Select 2nd UICC slot");
                    if(mNfcSecAdapter.setSecNfcPreferredSimSlot(0x02) == false)
                        mNfcSecAdapter.setSecNfcPreferredSimSlot(0x02);
                }
            }
        });
/* END [P1604040001] - Support Dual-SIM solution */

/* START [P1605200001] - Support One-Touch Payment solution */
        // Set Listener to One-Touch Payment Selection RadioGroup
        mRadioGroupPAY.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if(checkedId==R.id.rdbtn_pay_ready) {
                    //TODO : Put Payment Ready API here
                    Log.d(TAG, "One-Touch Payment ... (Ready)");
                    if(mNfcSecAdapter.setSecNfcOneTouchPayReady() == false)
                        mNfcSecAdapter.setSecNfcOneTouchPayReady();
                }
                else if(checkedId==R.id.rdbtn_pay_exec) {
                    //TODO : Put Payment Execute API here
                    Log.d(TAG, "One-Touch Payment ... (Execution)");
                    route = 0x02;    // UICC

                    if(mNfcSecAdapter.setSecNfcOneTouchPayExecute(route) == false)
                        mNfcSecAdapter.setSecNfcOneTouchPayExecute(route);
                }
            }
        });
/* END [P1605200001] - Support One-Touch Payment solution */

/* START [P1605310001] - Support Dynamic RF Selection */
        mRadioButtonRfTechA = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techa);
        mRadioButtonRfTechB = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techb);
        mRadioButtonRfTechAB = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techab);
        mRadioButtonRfTechF = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techf);
        mRadioButtonRfTechAll = (RadioButton) mView.findViewById(R.id.rdbtn_rf_techall);
        mRadioButtonRfTechNull = (RadioButton) mView.findViewById(R.id.rdbtn_rf_technull);

        mRadioGroupRF = (RadioGroup) mView.findViewById(R.id.radiogroup_rfselect);

        mRadioGroupRF.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if(checkedId==R.id.rdbtn_rf_techa) {
                    //TODO : Put Payment Ready API here
                    Log.d(TAG, "Select the listen tech-A only");
                    if(mNfcSecAdapter.setSecNfcListenTech(techA) == false)
                        mNfcSecAdapter.setSecNfcListenTech(techA);
                }
                else if(checkedId==R.id.rdbtn_rf_techb) {
                    //TODO : Put Payment Execute API here
                    Log.d(TAG, "Select the listen tech-B only");
                    if(mNfcSecAdapter.setSecNfcListenTech(techB) == false)
                        mNfcSecAdapter.setSecNfcListenTech(techB);
                }
                else if(checkedId==R.id.rdbtn_rf_techf) {
                    //TODO : Put Payment Execute API here
                    Log.d(TAG, "Select the listen tech-F only");
                    if(mNfcSecAdapter.setSecNfcListenTech(techF) == false)
                        mNfcSecAdapter.setSecNfcListenTech(techF);
                }
                else if(checkedId==R.id.rdbtn_rf_techab) {
                    //TODO : Put Payment Execute API here
                    Log.d(TAG, "Select the listen tech-A/B");
                    if(mNfcSecAdapter.setSecNfcListenTech(techA | techB) == false)
                        mNfcSecAdapter.setSecNfcListenTech(techA | techB);
                }
                else if(checkedId==R.id.rdbtn_rf_techall) {
                    //TODO : Put Payment Execute API here
                    Log.d(TAG, "Select the listen A/B/F All");
                    if(mNfcSecAdapter.setSecNfcListenTech(techA | techB | techF) == false)
                        mNfcSecAdapter.setSecNfcListenTech(techA | techB | techF);
                }
                else if(checkedId==R.id.rdbtn_rf_technull) {
                    //TODO : Put Payment Execute API here
                    Log.d(TAG, "Disable all listen techs.");
                    if(mNfcSecAdapter.setSecNfcListenTech(0x00) == false)
                        mNfcSecAdapter.setSecNfcListenTech(0x00);
                }
            }
        });
/* END [P1605310001] - Support Dynamic RF Selection */

        return mView;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        SettingsActivity activity = (SettingsActivity) getActivity();

        mSwitchBar = activity.getSwitchBar();
        mSwitchBar.setChecked(mNfcAdapter.isEnabled());
        mSwitchBar.addOnSwitchChangeListener(this);
        mSwitchBar.setEnabled(true);
        mSwitchBar.show();

    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mOldActivityTitle != null) {
            getActivity().getActionBar().setTitle(mOldActivityTitle);
        }
        mSwitchBar.removeOnSwitchChangeListener(this);
        mSwitchBar.hide();
    }

    @Override
    public void onSwitchChanged(Switch switchView, boolean desiredState) {
        boolean success = false;
        mSwitchBar.setEnabled(false);
        if (desiredState) {
            success = mNfcAdapter.enable();
        } else {
            success = mNfcAdapter.disable();
        }
        if (success) {
            mSwitchBar.setChecked(desiredState);
        }
        mSwitchBar.setEnabled(true);
    }

    private void clearCheckedRadioButtons()
    {
        mRadioGroupSE.clearCheck();
        mRadioGroupUICC.clearCheck();
        mRadioGroupPAY.clearCheck();
        mRadioGroupRF.clearCheck();
    }

    private void enableRadioButtons(boolean state)
    {
        mRadioButtonHce.setEnabled(state);
        mRadioButtonUicc.setEnabled(state);
        mRadioButtonEse.setEnabled(state);
        mRadioButtonUiccSel1.setEnabled(state);
        mRadioButtonUiccSel2.setEnabled(state);
        mRadioButtonPayReady.setEnabled(state);
        mRadioButtonPayExec.setEnabled(state);
        mRadioButtonRfTechA.setEnabled(state);
        mRadioButtonRfTechB.setEnabled(state);
        mRadioButtonRfTechAB.setEnabled(state);
        mRadioButtonRfTechF.setEnabled(state);
        mRadioButtonRfTechAll.setEnabled(state);
        mRadioButtonRfTechNull.setEnabled(state);
    }

    private void handleNfcStateChanged(int newState) {
        switch (newState) {
            case NfcAdapter.STATE_OFF:
                mSwitchBar.setEnabled(true);
                enableRadioButtons(false);    // Disable UI when NFC is Off
                break;
            case NfcAdapter.STATE_ON:
                mSwitchBar.setEnabled(true);
                enableRadioButtons(true);
                break;
            case NfcAdapter.STATE_TURNING_ON:
                mSwitchBar.setEnabled(false);
                enableRadioButtons(false);
                break;
            case NfcAdapter.STATE_TURNING_OFF:
                mSwitchBar.setEnabled(false);
                enableRadioButtons(false);
                break;
        }
    }
}
