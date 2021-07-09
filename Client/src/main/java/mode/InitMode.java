package mode;

import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class InitMode extends BaseMode {
	InitMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Init";
	}
	
	@Override
	public void start() {
		appendUiLog("Init Mode start");
		NetworkManager manager = NetworkManager.getInstance();
		if (manager.isReady()) {
			appendUiLog("Already Initialized");
			return;
		}
		NetworkManager.getInstance().init();
	}

	@Override
	public void stop() {
		appendUiLog("Init Mode stop");
		PlaybackManager.getInstance().stop();
	}

}
