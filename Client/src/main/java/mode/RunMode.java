package mode;

import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

import java.net.Socket;

public class RunMode extends BaseMode {
	private final PlaybackManager playbackManager;

	RunMode(UiController uiController) {
		super(uiController);

		playbackManager = new PlaybackManager(uiController);
	}

	@Override
	public String getModeName() {
		return "Run Mode";
	}
	
	@Override
	public void start() {
		NetworkManager networkManager = NetworkManager.getInstance();
		Socket socket = networkManager.getNanoSocket();
		if (socket == null) {
			appendUiLog("Run Mode failed.");
			return;
		}

		appendUiLog("Run Mode start");
		playbackManager.play(socket);
	}

	@Override
	public void stop() {
		appendUiLog("Run Mode stop");
		playbackManager.stop();
	}

}
