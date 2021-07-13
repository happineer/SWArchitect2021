package com.lge.cmuteam3.client.network;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.FileProperties;
import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.Receiver;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class NetworkManager {
	private static final Logger LOG = LoggerFactory.getLogger(NetworkManager.class);
	private static NetworkManager instance;
	
	private Socket nanoSocket;
	private Receiver receiver;
	private OnServerStateListener onServerStateListener;
	private boolean isReady = false;
	private int retryCount = 0;
	
	private final ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
	
	private NetworkManager() {}
	public static NetworkManager getInstance() {
		if (instance == null) {
			instance = new NetworkManager();
		}
		return instance;
	}
	
	public synchronized void initialize() {
		serviceUnavailable(Constants.CONNECTION_STATE_CONNECTING);
		try {
			for (int i = 0; i < 3; i++) {
				init();
				if (isReady) {
					return;
				}
				Thread.sleep(3000);
			}
			serviceUnavailable(Constants.CONNECTION_STATE_FAILED);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	private void init() {
		if (isReady) {
			serviceReady();
			return;
		}
		FileProperties prop = FileProperties.getInstance();
		String serverIp = prop.getProperty("server.ip");
		int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));

		try {
			String serverInfo = serverIp + ":" + serverTransferPort;
			LOG.info(serverInfo);
			
			NetworkUiLogManager.append("Try to connect : " + serverInfo);
			nanoSocket = new Socket(serverIp, serverTransferPort);
			receiver = new Receiver(nanoSocket);
			System.out.println("gaenoo socket connect");
			serviceReady();
			
		} catch (Exception e) {
			String msg = "Connection failed! : [" + e.getMessage() + "]";
			LOG.info(msg);
			NetworkUiLogManager.append(msg);
			serviceUnavailable(1);
		}
	}

	public void reInitialize() {
		isReady = false;
		initialize();
	}

	private void serviceReady() {
		isReady = true;
		if (onServerStateListener != null) {
			onServerStateListener.onReady();
		}
	}

	private void serviceUnavailable(int state) {
		isReady = false;
		if (onServerStateListener != null) {
			onServerStateListener.onFail(state);
		}
	}

	public boolean isReady() {
		return isReady;
	}

	public Receiver getReceiver() {
		return receiver;
	}

	public void controlNano(int type, int value) {
		if (nanoSocket == null) {
			LOG.info("Socket is null.");
			return;
		}
		try {
			BufferedOutputStream bs = new BufferedOutputStream(nanoSocket.getOutputStream());
			bs.write(type);
			bs.write(0x0);
			bs.write(0x0);
			bs.write(0x0);
			bs.write(value);
			bs.write(0x0);
			bs.write(0x0);
			bs.write(0x0);
			bs.flush();
		} catch (IOException e) {
			serviceUnavailable(1);
			e.printStackTrace();
		}
	}
	
	public void setOnServerStateListener(OnServerStateListener onServerStateListener) {
		this.onServerStateListener = onServerStateListener;
	}

	public void disconnect() {
		if (nanoSocket != null) {
			try {
				serviceUnavailable(0);
				nanoSocket.close();
				nanoSocket = null;
			} catch (IOException e) {
				LOG.error("Socket Exception:" + e.getMessage());
			}
		}
	}
}
