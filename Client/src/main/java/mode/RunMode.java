package mode;

import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class RunMode extends BaseMode {
	RunMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Run";
	}
	
	@Override
	public void start() {
		NetworkManager networkManager = NetworkManager.getInstance();
		appendUiLog("Run Mode start");
		PlaybackManager.getInstance().play();
		networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_RUN);
	}

	@Override
	public void stop() {
		appendUiLog("Run Mode stop");
		PlaybackManager.getInstance().stop();
	}

}
