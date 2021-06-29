package com.lge.cmuteam3.client;

public class PlaybackManager {

	private Player player;
	private Receiver receiver;
	private static PlaybackManager instance;
	
	private PlaybackManager() {
		FileProperties prop = FileProperties.getInstance();
		String serverIp = prop.getProperty("server.ip");
		int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
		receiver = new Receiver(serverIp, serverTransferPort);
		player = new Player(receiver);
	}
	
	public static PlaybackManager getInstance() {
		if (instance == null) {
			instance = new PlaybackManager();
		}
		return instance;
	}
	
	
	public void play() {
		player.start();
	}
}
