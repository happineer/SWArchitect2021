package com.lge.cmuteam3.client.network;

import java.io.IOException;
import java.net.Socket;
import com.lge.cmuteam3.client.FileProperties;

public class NetworkManager {

	Socket nanoSocket;
	Socket controllerSocket;
	
	private static NetworkManager instance = new NetworkManager();
	private NetworkManager() {}
	public static NetworkManager getInstance() {
		return instance;
	}
	
	// 초기화 추가 변경 필요
	public boolean init() {
		FileProperties prop = FileProperties.getInstance();
		String serverIp = prop.getProperty("server.ip");
		int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
		int controlPort = Integer.parseInt(prop.getProperty("server.control.port"));
		
		
		try {
			nanoSocket = new Socket(serverIp, serverTransferPort);
//			controllerSocket = new Socket(serverIp, controlPort);
			return true;
		} catch (Exception e) {
			e.printStackTrace();
			instance = null;
			return false;
		}
		
		
	}
	
	public void controlNano() {
		
	}
	
	
	public Socket getNanoSocket() {
		return nanoSocket;
	}
	
}
