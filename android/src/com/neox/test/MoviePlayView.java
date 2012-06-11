package com.neox.test;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

public class MoviePlayView extends View {
    private Bitmap mBitmap;
	private Context mContext;
	private boolean isOpen;

    public MoviePlayView(Context context) {
        super(context);
        
        mContext = context;
        
        Log.d("ffmpeg", "MoviePlayView()");
        
        if (initBasicPlayer() < 0) {
        	Toast.makeText(context, "CPU doesn't support NEON", Toast.LENGTH_LONG).show();
        	
        	((Activity)context).finish();
        }
        
        
    }

    @Override
    protected void onDraw(Canvas canvas) {
    	if(isOpen) {
	    	renderFrame(mBitmap);
	        canvas.drawBitmap(mBitmap, 0, 0, null);
	
	        invalidate();
    	}
    }

    public void playMovie(String path) {
    	isOpen = false;
        int openResult = openMovie(path);
        if (openResult < 0) {
        	Toast.makeText(mContext, "Open Movie Error: " + openResult, Toast.LENGTH_LONG).show();
        	
//        	((Activity)mContext).finish();
        }
        else {
        	mBitmap = Bitmap.createBitmap(getMovieWidth(), getMovieHeight(), Bitmap.Config.RGB_565);
        	isOpen = true;
        }
    }
    
    public void stopMovie() {
    	closeMovie();
    }
    
    static {
        System.loadLibrary("basicplayer");
    }

    public static native int initBasicPlayer();
	public static native int openMovie(String filePath);
	public static native int renderFrame(Bitmap bitmap);
	public static native int getMovieWidth();
	public static native int getMovieHeight();
	public static native void closeMovie();

}
