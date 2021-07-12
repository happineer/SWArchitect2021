package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.ui.UiController;

public class PlaybackManager {

	private static PlaybackManager instance;
	private Player player;
	
	public static void initialize(UiController uiController) {
		if (instance == null) {
			instance = new PlaybackManager(uiController);
		}
	}
	
	public static PlaybackManager getInstance() {
		return instance;
	}
	
	private PlaybackManager(UiController uiController) {
		player = new Player(uiController);
	}
	
	public void play() {
		player.start();
	}

	public void playDirect() {
		player.startDirect();
	}
	
	public void stop() {
		if (player != null) {
			player.stop();
		}
	}
	
	public Player getPlayer() {
		return player;
	}
}
