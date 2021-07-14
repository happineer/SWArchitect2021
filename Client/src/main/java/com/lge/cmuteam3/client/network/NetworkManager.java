package com.lge.cmuteam3.client.network;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.FileProperties;
import com.lge.cmuteam3.client.Receiver;
import mode.Mode;
import mode.ModeManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class NetworkManager {
	private static final Logger LOG = LoggerFactory.getLogger(NetworkManager.class);
	private static NetworkManager instance;
	
	private final int retryCount = 100;
	private Socket nanoSocket;
	private Receiver receiver;
	private OnServerStateListener onServerStateListener;
	private boolean isReady = false;
	private Mode lastMode = null;
	
	private final ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
	
	private NetworkManager() {}
	public static NetworkManager getInstance() {
		if (instance == null) {
			instance = new NetworkManager();
		}
		return instance;
	}
	
	public synchronized void initialize(int reason) {
		serviceUnavailable(reason);
		try {
			for (int i = 0; i < retryCount; i++) {
				init(reason);
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
	
	private void init(int reason) {
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
			
			if (receiver != null) {
				receiver.terminate();
			}
			
			NetworkUiLogManager.append("Try to connect : " + serverInfo);
			nanoSocket = new Socket(serverIp, serverTransferPort);
			receiver = new Receiver(nanoSocket);
			serviceReady();
		} catch (Exception e) {
			String msg = "Connection failed! : [" + e.getMessage() + "]";
			LOG.info(msg);
			NetworkUiLogManager.append(msg);
			serviceUnavailable(reason);
		}
	}

	public void reInitialize(int reason) {
		isReady = false;
		lastMode = ModeManager.getInstance().getCurrentMode(null);
		initialize(reason);
	}

	private void serviceReady() {
		isReady = true;
		if (onServerStateListener != null) {
			onServerStateListener.onReady();
		}
		if (lastMode != null && lastMode.needRestore()) {
			ModeManager.getInstance().onUiStart(lastMode);
			lastMode = null;
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

	public void disconnect(int reason) {
		if (nanoSocket != null) {
			try {
				nanoSocket.close();
				nanoSocket = null;
			} catch (IOException e) {
				LOG.error("Socket Exception:" + e.getMessage());
			}
			
			serviceUnavailable(reason);
			
			executor.schedule(()-> {
				reInitialize(reason);
			}, 5, TimeUnit.SECONDS);
		}
	}
}
