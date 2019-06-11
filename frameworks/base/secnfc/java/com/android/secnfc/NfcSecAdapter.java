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

package com.android.secnfc;

import java.util.HashMap;

import android.content.Context;
import android.nfc.INfcSecAdapter;
import android.nfc.NfcAdapter;
import android.os.RemoteException;
import android.util.Log;

/**
 * Provides additional methods on an {@link NfcAdapter} for This Package
  *
 * There is a 1-1 relationship between an {@link NfcSecAdapter} object and
 * a {@link NfcAdapter} object.
 */
public final class NfcSecAdapter {
    private static final String TAG = "NfcSecAdapter";

    // protected by NfcSecAdapter.class, and final after first construction,
    // except for attemptDeadServiceRecovery() when NFC crashes - we accept a
    // best effort recovery
    private static INfcSecAdapter sService;

    // contents protected by NfcSecAdapter.class
    private static final HashMap<NfcAdapter, NfcSecAdapter> sNfcSec = new HashMap();

    private final NfcAdapter mAdapter;

    /** get service handles */
    private static void initService(NfcAdapter adapter) {
        final INfcSecAdapter service = adapter.getNfcSecAdapterInterface();
        if (service != null) {
            // Leave stale rather than receive a null value.
            sService = service;
        }
    }

    /**
     * Get the {@link NfcSecAdapter} for the given {@link NfcAdapter}.
     *
     * <p class="note">
     * Requires the {@link android.Manifest.permission#WRITE_SECURE_SETTINGS} permission.
     *
     * @param adapter a {@link NfcAdapter}, must not be null
     * @return the {@link NfcSecAdapter} object for the given {@link NfcAdapter}
     */
    public static NfcSecAdapter get(NfcAdapter adapter) {
        Log.d(TAG, "Enter NfcSecAdapter.get() function.");
        Context context = adapter.getContext();
        if (context == null) {
            throw new UnsupportedOperationException(
                    "You must pass a context to your NfcAdapter to use the NFC SEC APIs");
        }

        synchronized (NfcSecAdapter.class) {
            if (sService == null) {
                initService(adapter);
            }
            NfcSecAdapter nfcsec = sNfcSec.get(adapter);
            if (nfcsec == null) {
                nfcsec = new NfcSecAdapter(adapter);
                sNfcSec.put(adapter,  nfcsec);
            }
            return nfcsec;
        }
    }

    private NfcSecAdapter(NfcAdapter adapter) {
        mAdapter = adapter;
    }

    /**
     * NFC service dead - attempt best effort recovery
     */
    void attemptDeadServiceRecovery(Exception e) {
        Log.e(TAG, "NFC SEC Adapter dead - attempting to recover");
        mAdapter.attemptDeadServiceRecovery(e);
        initService(mAdapter);
    }

    INfcSecAdapter getService() {
        return sService;
    }

/* START [P1604040001] - Support Dual-SIM solution */
    public boolean setSecNfcPreferredSimSlot(int preferedSimSlot){
        try {
            Log.d(TAG, "enter setSecNfcPreferredSimSlot() ");
            sService.setPreferredSimSlot(preferedSimSlot);
            return true;
        }
        catch(RemoteException e) {
            attemptDeadServiceRecovery(e);
            return false;
        }
    }
/* END [P1604040001] - Support Dual-SIM solution */

/* START [P160421001] - Patch for Dynamic SE Selection */
    public boolean initSecNfcPaymentSetting(int tech, int proto){
        try {
            Log.d(TAG, "enter initSecNfcPaymentSetting ");
            sService.clearListenModeRouting(tech, proto);
            return true;
        }
        catch(RemoteException e) {
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    public boolean setSecNfcPaymentSetting(int type, int value, int route, int power){
        try {
            Log.d(TAG, "enter setSecNfcPaymentSetting ");
            sService.setListenModeRouting(type, value, route, power);
            return true;
        }
        catch(RemoteException e) {
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    public boolean applySecNfcPaymentSettingOnly(){
        try {
            Log.d(TAG, "enter applySecNfcPaymentSettingOnly ");
            sService.commitListenModeRoutingOnly();
            return true;
        }
        catch(RemoteException e) {
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    public boolean applySecNfcPaymentSettingAndReset(){
        try {
            Log.d(TAG, "enter applySecNfcPaymentSettingAndReset ");
            sService.commitListenModeRoutingAndReset();
            return true;
        }
        catch(RemoteException e) {
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    /**
     * Set the default SE for routing based on protocol (ISO_DEP).
     * Also this function is for enable card emulation for execute one-touch payement.
     *
     * @param is SE_ID for default routing.
     */
    public boolean setSecNfcDefaultRoute(int defaultSE){
        Log.d(TAG, "setSecNfcDefaultRoute() .... enter.");

        int techValue = 0x03;
        int protoValue = 0x01;
        int powerValue = 0x3F;

        try {
            sService.clearListenModeRouting(0x01, 0x02);
            sService.setListenModeRouting(0x01, techValue, defaultSE, powerValue);
            sService.setListenModeRouting(0x02, protoValue, defaultSE, powerValue);
            sService.commitListenModeRoutingAndReset();

            Log.d(TAG, "setSecNfcDefaultRoute() .... exit.");
            return true;
        }
        catch(RemoteException e) {
            Log.d(TAG, "setSecNfcDefaultRoute() .... fail.");
            attemptDeadServiceRecovery(e);
            return false;
        }
    }
/* END [P160421001] - Patch for Dynamic SE Selection */

/* START [P1605200001] - Support One-Touch Payment solution */
    /**
     * Disable routing about listen tech_A and tech_B.
     *
     */
    public boolean setSecNfcOneTouchPayReady(){
        Log.d(TAG, "setSecNfcOneTouchPayReady() .... enter.");

        int techValue = 0x03;
        int protoValue = 0x01;
        int powerValue = 0x3F;

        try {
            sService.clearListenModeRouting(0x01, 0x02);
            sService.commitListenModeRoutingOnly();

            Log.d(TAG, "setSecNfcOneTouchPayReady() .... exit.");
            return true;
        }
        catch(RemoteException e) {
            Log.d(TAG, "setSecNfcOneTouchPayReady() .... fail.");
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    /**
     *This function is for enable card emulation for execute one-touch payement.
     *
     * @param is SE_ID for default routing.
     */
    public boolean setSecNfcOneTouchPayExecute(int defaultSE){
        Log.d(TAG, "setSecNfcOneTouchPayExecute() .... enter.");

        int techValue = 0x03;
        int protoValue = 0x01;
        int powerValue = 0x3F;

        try {
            sService.clearListenModeRouting(0x01, 0x02);
            sService.setListenModeRouting(0x01, techValue, defaultSE, powerValue);
            sService.setListenModeRouting(0x02, protoValue, defaultSE, powerValue);
            sService.commitListenModeRoutingOnly();

            Log.d(TAG, "setSecNfcOneTouchPayExecute() .... exit.");
            return true;
        }
        catch(RemoteException e) {
            Log.d(TAG, "setSecNfcOneTouchPayExecute() .... fail.");
            attemptDeadServiceRecovery(e);
            return false;
        }
    }
/* END [P1605200001] - Support One-Touch Payment solution */

/* START [16052901F] - Change listen tech mask values */
    /**
     *This function is to configurate listen tech.
     *
     * @param is a mask for setting listen tech valeu.
     */
    public boolean setSecNfcListenTech(int listen_tech){
        try {
            Log.d(TAG, "setSecNfcListenTech ... enter.");
            sService.changeListenTechMask(listen_tech);
            return true;
        }
        catch(RemoteException e) {
            attemptDeadServiceRecovery(e);
            return false;
        }
    }
/* END [16052901F] - Change listen tech mask values */
}
