package com.jfc.util.misc;

import com.jfc.apps.hive.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.View;

public class DialogUtils {
	public static AlertDialog createAndShowAlertDialog(Activity activity, 
													   String msg,
													   int posResId, final Runnable posAction, 
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(activity, null, R.drawable.ic_alert, activity.getString(R.string.warning), msg, posResId, posAction, 0, null, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowErrorDialog(Activity activity, 
													   String msg, 
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(activity, null, R.drawable.ic_alert, activity.getString(R.string.error), msg, 0, null, 0, null, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowAlertDialog(Activity activity, 
													   int msgResId,
													   int posResId, final Runnable posAction, 
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(activity, null, R.drawable.ic_alert, R.string.warning, msgResId, posResId, posAction, 0, null, cancelResId, cancelAction);
	}
	
	public static AlertDialog createAndShowAlertDialog(Activity activity, 
													   String msg,
													   int posResId, final Runnable posAction, 
													   int neutralResId, final Runnable neutralAction,
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(activity, null, R.drawable.ic_alert, activity.getString(R.string.warning), msg, posResId, posAction, neutralResId, neutralAction, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowInfoDialog(Activity activity, 
													  int layoutResId,
													  int neutralResId, final Runnable neutralAction,
													  int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(activity, View.inflate(activity, layoutResId, null), 0, 0, 0, cancelResId, cancelAction, neutralResId, neutralAction, 0, cancelAction);
	}
	
	public static AlertDialog createAndShowInfoDialog(Activity activity, 
													  int layoutResId,
													  int posResId, final Runnable posAction,
													  int neutralResId, final Runnable neutralAction,
													  int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(activity, View.inflate(activity, layoutResId, null), 0, 0, 0, neutralResId, neutralAction, posResId, posAction, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowDialog(Activity activity, 
												  View view, int iconResId,
												  int titleResId, int msgResId,
												  int posResId, final Runnable posAction, 
												  int neutralResId, final Runnable neutralAction,
												  int cancelResId, final Runnable cancelAction) {
		String title = null, msg = null;
		if (titleResId > 0) title = activity.getString(titleResId);
		if (msgResId > 0) msg = activity.getString(msgResId);
		return createAndShowDialog(activity, view, iconResId, title, msg, posResId, posAction, neutralResId, neutralAction, cancelResId, cancelAction);
	}
	
	public static AlertDialog createAndShowDialog(Activity activity, 
												   View view, int iconResId, 
												   String title, String msg,
												   int posResId, final Runnable posAction, 
												   int neutralResId, final Runnable neutralAction,
												   int cancelResId, final Runnable cancelAction) {
		AlertDialog.Builder builder = new AlertDialog.Builder(activity);
		if (iconResId > 0) builder.setIcon(iconResId);
		if (view != null) builder.setView(view);
		if (title != null) builder.setTitle(title);
		if (msg != null) builder.setMessage(msg);
		if (posResId > 0) 
			builder.setPositiveButton(posResId, new DialogInterface.OnClickListener() {
							@Override
							public void onClick(DialogInterface dialog, int which) {
								dialog.dismiss();
								if (posAction != null) posAction.run();
							}
						}
			);
		if (neutralResId > 0) 
			builder.setNeutralButton(neutralResId, new DialogInterface.OnClickListener() {
								@Override
								public void onClick(DialogInterface dialog, int which) {
									dialog.dismiss();
									if (neutralAction != null) neutralAction.run();
								}
							}
			);
		if (cancelResId > 0) 
			builder.setNegativeButton(cancelResId, new DialogInterface.OnClickListener() {
								@Override
								public void onClick(DialogInterface dialog, int which) {
									dialog.dismiss();
									if (cancelAction != null) cancelAction.run();
								}
							}
			);
		if (cancelAction == null) 
			throw new IllegalArgumentException("A cancel action must be supplied to DialogUtils.createAndShowDialog");
		builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
										@Override
										public void onCancel(DialogInterface dialog) {cancelAction.run();}
									}
		);
		
		AlertDialog alert = builder.create();
		if (!activity.isFinishing())
			alert.show();
		
		return alert;
	}
	
	public static AlertDialog createAndShowDialog(Context context, 
												  View view, int iconResId, 
												  String title, String msg,
												  String posRes, final Runnable posAction, 
												  String neutralRes, final Runnable neutralAction,
												  int cancelResId, final Runnable cancelAction) {
		AlertDialog.Builder builder = new AlertDialog.Builder(context);
		if (iconResId > 0) builder.setIcon(iconResId);
		if (view != null) builder.setView(view);
		if (title != null) builder.setTitle(title);
		if (msg != null) builder.setMessage(msg);
		if (posRes != null) 
			builder.setPositiveButton(posRes, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						dialog.dismiss();
						if (posAction != null) posAction.run();
					}
				}
			);
		if (neutralRes != null) 
			builder.setNeutralButton(neutralRes, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						dialog.dismiss();
						if (neutralAction != null) neutralAction.run();
					}
				}
			);
		if (cancelResId > 0) 
			builder.setNegativeButton(cancelResId, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						dialog.dismiss();
						if (cancelAction != null) cancelAction.run();
					}
				}
			);
		if (cancelAction == null) 
			throw new IllegalArgumentException("A cancel action must be supplied to DialogUtils.createAndShowDialog");
		builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {cancelAction.run();}
			}
		);
		
		AlertDialog alert = builder.create();
		alert.show();
		
		return alert;
	}

}
