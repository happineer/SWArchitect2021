package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.ui.UiController;

import java.net.Socket;

public class PlaybackManager {

	private Player player;
	private Receiver receiver;
	private UiController uiController;

	public PlaybackManager(UiController uiController) {
		this.uiController = uiController;
	}
	
	public void play(Socket socket) {
		receiver = new Receiver(socket);
		player = new Player(receiver, uiController);
		player.start(socket);
	}

	public void stop() {
		if (player != null) {
			player.stop();
		}
	}
}
