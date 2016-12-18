package com.jfc.util.misc;

import com.example.hive.R;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.View;

public class DialogUtils {
	public static AlertDialog createAndShowAlertDialog(Context context, 
													   String msg,
													   int posResId, final Runnable posAction, 
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(context, null, R.drawable.ic_alert, context.getString(R.string.warning), msg, posResId, posAction, 0, null, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowErrorDialog(Context context, 
													   String msg, 
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(context, null, R.drawable.ic_alert, context.getString(R.string.error), msg, 0, null, 0, null, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowAlertDialog(Context context, 
													   int msgResId,
													   int posResId, final Runnable posAction, 
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(context, null, R.drawable.ic_alert, R.string.warning, msgResId, posResId, posAction, 0, null, cancelResId, cancelAction);
	}
	
	public static AlertDialog createAndShowAlertDialog(Context context, 
													   String msg,
													   int posResId, final Runnable posAction, 
													   int neutralResId, final Runnable neutralAction,
													   int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(context, null, R.drawable.ic_alert, context.getString(R.string.warning), msg, posResId, posAction, neutralResId, neutralAction, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowInfoDialog(Context context, 
													  int layoutResId,
													  int neutralResId, final Runnable neutralAction,
													  int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(context, View.inflate(context, layoutResId, null), 0, 0, 0, cancelResId, cancelAction, neutralResId, neutralAction, 0, cancelAction);
	}
	
	public static AlertDialog createAndShowInfoDialog(Context context, 
													  int layoutResId,
													  int posResId, final Runnable posAction,
													  int neutralResId, final Runnable neutralAction,
													  int cancelResId, final Runnable cancelAction) {
		return createAndShowDialog(context, View.inflate(context, layoutResId, null), 0, 0, 0, neutralResId, neutralAction, posResId, posAction, cancelResId, cancelAction);
	}

	public static AlertDialog createAndShowDialog(Context context, 
												  View view, int iconResId,
												  int titleResId, int msgResId,
												  int posResId, final Runnable posAction, 
												  int neutralResId, final Runnable neutralAction,
												  int cancelResId, final Runnable cancelAction) {
		String title = null, msg = null;
		if (titleResId > 0) title = context.getString(titleResId);
		if (msgResId > 0) msg = context.getString(msgResId);
		return createAndShowDialog(context, view, iconResId, title, msg, posResId, posAction, neutralResId, neutralAction, cancelResId, cancelAction);
	}
	
	public static AlertDialog createAndShowDialog(Context context, 
												   View view, int iconResId, 
												   String title, String msg,
												   int posResId, final Runnable posAction, 
												   int neutralResId, final Runnable neutralAction,
												   int cancelResId, final Runnable cancelAction) {
		AlertDialog.Builder builder = new AlertDialog.Builder(context);
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
