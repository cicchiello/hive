package com.jfc.util.misc;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import android.app.Activity;
import android.widget.TextView;

import com.jfc.apps.hive.HiveEnv;

public class SplashyText {
	static private Map<TextView,AtomicInteger> sActiveTVs = new HashMap<TextView,AtomicInteger>();
	
	public static void highlightModifiedField(final Activity activity, final TextView t) {
		Runnable flair = new Runnable() {
			@Override
			public void run() {
				if (sActiveTVs.containsKey(t)) {
					sActiveTVs.get(t).set(0);
				} else {
					final int numSteps = 10;
					final int blastColor = HiveEnv.ModifiableFieldSplashColor;
					final int finalColor = HiveEnv.ModifiableFieldBackgroundColor;
					int sr = ((blastColor >> 16) & 0xff), sg = ((blastColor >> 8) & 0xff), sb = ((blastColor >> 0) & 0xff);
					int fr = (finalColor&0x00ff0000 >> 16), fg = (finalColor&0x0000ff00 >> 8), fb = (finalColor&0x000000ff);
					float dr = (fr-sr)/(numSteps*1f), dg = (fg-sg)/(numSteps*1f), db = (fb-sb)/(numSteps*1f);
					
					AtomicInteger ai = new AtomicInteger(0);
					sActiveTVs.put(t, ai);
					for (; ai.get() < numSteps; ai.incrementAndGet()) {
						int i = ai.get();
						int nr = sr+((int) (i*dr+0.5));
						int ng = sg+((int) (i*dg+0.5));
						int nb = sb+((int) (i*db+0.5));
						final int c = 0xff000000 + (nr << 16) + (ng << 8) + nb;
						activity.runOnUiThread(new Runnable() {
							@Override
							public void run() {
								t.setBackgroundColor(c);
							}
						});
						try {
							Thread.sleep(200);
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}
					sActiveTVs.remove(t);
					activity.runOnUiThread(new Runnable() {
						@Override
						public void run() {
							t.setBackgroundColor(finalColor);
						}
					});
				}
			}
		};
		Thread flairThread = new Thread(flair);
		flairThread.start();
	}
	
	public static void highlightErrorField(Activity activity, TextView t) {
		t.setBackgroundColor(HiveEnv.ModifiableFieldErrorColor);
	}
	

}
