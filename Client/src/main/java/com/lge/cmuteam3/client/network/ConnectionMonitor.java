package com.lge.cmuteam3.client.network;

import java.util.Timer;
import java.util.TimerTask;

public class ConnectionMonitor {

	private String ip;
	private int controlPort;
	private Checker checker;
	
	public ConnectionMonitor(String ip, int controlPort) {
		this.ip = ip;
		this.controlPort = controlPort;
	}

	private class Checker extends TimerTask {

		@Override
		public void run() {
			// TODO Auto-generated method stub
			
		}
		
	}
	
	
	public void checkConnection() {
		if (checker != null) {
			checker = new Checker();
		}
		checker.cancel();
        Timer timer = new Timer();
        timer.schedule(checker, 0, 300);
	}
	
	public void stop() {
		
	}
	
}
