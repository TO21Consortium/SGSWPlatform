/*
 * Copyright 2015, The Android Open Source Project
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

package com.android.cts.verifier.managedprovisioning;

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.net.Uri;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;
import android.widget.VideoView;
import android.view.Display;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.content.ContentResolver;
import com.android.cts.verifier.R;

import java.io.IOException;
import java.io.InputStream;

/**
 * This dialog shows/plays an image, video or audio uri.
 */
public class ByodPresentMediaDialog extends DialogFragment {
    static final String TAG = "ByodPresentMediaDialog";

    private static final String KEY_VIDEO_URI = "video";
    private static final String KEY_IMAGE_URI = "image";
    private static final String KEY_AUDIO_URI = "audio";

    private Bitmap scaled = null;
    /**
     * Get a dialogFragment showing an image.
     */
    public static ByodPresentMediaDialog newImageInstance(Uri uri) {
        ByodPresentMediaDialog dialog = new ByodPresentMediaDialog();
        Bundle args = new Bundle();
        args.putParcelable(KEY_IMAGE_URI, uri);
        dialog.setArguments(args);
        return dialog;
    }

    /**
     * Get a dialogFragment playing a video.
     */
    public static ByodPresentMediaDialog newVideoInstance(Uri uri) {
        ByodPresentMediaDialog dialog = new ByodPresentMediaDialog();
        Bundle args = new Bundle();
        args.putParcelable(KEY_VIDEO_URI, uri);
        dialog.setArguments(args);
        return dialog;
    }

    /**
     * Get a dialogFragment playing audio.
     */
    public static ByodPresentMediaDialog newAudioInstance(Uri uri) {
        ByodPresentMediaDialog dialog = new ByodPresentMediaDialog();
        Bundle args = new Bundle();
        args.putParcelable(KEY_AUDIO_URI, uri);
        dialog.setArguments(args);
        return dialog;
    }

    private int calculateInSampleSize(BitmapFactory.Options options, int reqWidth, int reqHeight){
        // Raw height and width of image
        final int height = options.outHeight;
        final int width = options.outWidth;
        if(reqWidth <= 0 || reqHeight <= 0) {
           return 1;
        }
        return Math.max(height/reqHeight, width/reqWidth) + 1;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Dialog dialog = new Dialog(getActivity());
        dialog.setContentView(R.layout.byod_present_media);

        Button dismissButton = (Button) dialog.findViewById(R.id.dismissButton);
        dismissButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dismiss();
                ((DialogCallback) getActivity()).onDialogClose();
            }
        });

        Bundle arguments = getArguments();

        // Initially all video and image specific UI is invisible.
        if (arguments.containsKey(KEY_VIDEO_URI)) {
            // Show video UI.
            dialog.setTitle(getString(R.string.provisioning_byod_verify_video_title));

            Uri uri = (Uri) getArguments().getParcelable(KEY_VIDEO_URI);
            final VideoView videoView = (VideoView) dialog.findViewById(R.id.videoView);
            videoView.setVisibility(View.VISIBLE);
            videoView.setVideoURI(uri);

            Button playButton = (Button) dialog.findViewById(R.id.playButton);
            playButton.setVisibility(View.VISIBLE);
            playButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    videoView.start();
                }
            });
        } else if (arguments.containsKey(KEY_IMAGE_URI)) {
            // Show image UI.
            dialog.setTitle(getString(R.string.provisioning_byod_verify_image_title));
            Uri uri = (Uri)getArguments().getParcelable(KEY_IMAGE_URI);
            ImageView imageView = (ImageView) dialog.findViewById(R.id.imageView);
            imageView.setVisibility(View.VISIBLE);

            try{
                InputStream input = getActivity().getContentResolver().openInputStream(uri);
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inJustDecodeBounds = true;
                BitmapFactory.decodeStream(input, null, options);
                //scale the picture
                Display display = getActivity().getWindowManager().getDefaultDisplay();
                Point size = new Point();
                display.getSize(size);
                int reqWidth = size.x;
                int reqHeight = size.y;
                options.inSampleSize = calculateInSampleSize(options, reqWidth, reqHeight);

                options.inJustDecodeBounds = false;
                input.close();
                input = getActivity().getContentResolver().openInputStream(uri);
                scaled = BitmapFactory.decodeStream(input, null, options);
                input.close();
                imageView.setImageBitmap(scaled);
            }catch(IOException e){
                Log.e(TAG, "Cannot get image.", e);
                Toast.makeText(getActivity(),R.string.provisioning_byod_capture_image_error,
                        Toast.LENGTH_SHORT).show();
                getActivity().finish();
            }

        } else if (arguments.containsKey(KEY_AUDIO_URI)) {
            // Show audio playback UI.
            dialog.setTitle(getString(R.string.provisioning_byod_verify_audio_title));

            Uri uri = (Uri) getArguments().getParcelable(KEY_AUDIO_URI);
            final MediaPlayer mediaPlayer = new MediaPlayer();
            final Button playButton = (Button) dialog.findViewById(R.id.playButton);
            playButton.setVisibility(View.VISIBLE);
            playButton.setEnabled(false);

            try {
                mediaPlayer.setDataSource(getActivity(), uri);
                mediaPlayer.prepare();
            } catch (IllegalArgumentException|SecurityException|IllegalStateException
                    |IOException e) {
                Log.e(TAG, "Cannot play given audio with media player.", e);
                Toast.makeText(getActivity(), R.string.provisioning_byod_capture_media_error,
                        Toast.LENGTH_SHORT).show();
                getActivity().finish();
            }

            mediaPlayer.setOnPreparedListener(new OnPreparedListener() {
                @Override
                public void onPrepared(MediaPlayer mp) {
                    playButton.setEnabled(true);
                    playButton.setOnClickListener(new View.OnClickListener() {
                        public void onClick(View v) {
                            mediaPlayer.start();
                        }
                    });
                }
            });
        }

        return dialog;
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        ((DialogCallback) getActivity()).onDialogClose();
    }

    @Override
    public void onDestroyView() {
        if(scaled!=null){
            scaled.recycle();
        }
        super.onDestroyView();
    }
    public interface DialogCallback {
        public abstract void onDialogClose();
    }
}
