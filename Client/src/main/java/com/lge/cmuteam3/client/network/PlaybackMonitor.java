package com.lge.cmuteam3.client.network;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

public class PlaybackMonitor {

	private final ScheduledExecutorService executor;
	private ScheduledFuture<?> future;
	
	public PlaybackMonitor() {
		executor = Executors.newSingleThreadScheduledExecutor();
	}

	public void onNewFrame(int size) {
		if (future == null) {
			future = executor.schedule(this::handleResult, 5, TimeUnit.SECONDS);
			return;
		}
		
		if (future != null && size != 0) {
			future.cancel(true);
			future = executor.schedule(this::handleResult, 5, TimeUnit.SECONDS);
		}
		
	}
	
	public void handleResult() {
		NetworkManager.getInstance().reInitialize();
	}
	
}
