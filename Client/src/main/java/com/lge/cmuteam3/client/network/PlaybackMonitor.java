package com.lge.cmuteam3.client.network;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

public class PlaybackMonitor {

	private final ScheduledExecutorService executor;
	private ScheduledFuture<?> future;
	private boolean startMonitoring = false;
	
	public PlaybackMonitor() {
		executor = Executors.newSingleThreadScheduledExecutor();
	}

	public void start() {
		startMonitoring = true;
	}
	
	public void onNewFrame(int size) {
		if (!startMonitoring) {
			return;
		}
		
		if (future == null) {
			future = executor.schedule(this::handleResult, 10, TimeUnit.SECONDS);
			return;
		}
		if (future != null && size != 0) {
			future.cancel(false);
			future = executor.schedule(this::handleResult, 10, TimeUnit.SECONDS);
		}	
	}
	
	public void stop() {
		startMonitoring = false;
		if (future != null) {
			future.cancel(false);
		}
	}
	
	public void handleResult() {
		NetworkManager.getInstance().reInitialize();
	}
	
}
