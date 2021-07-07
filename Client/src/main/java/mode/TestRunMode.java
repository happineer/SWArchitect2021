package mode;

import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class TestRunMode extends BaseMode {

	public TestRunMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Test Run";
	}
	
	@Override
	public void start() {
		NetworkManager networkManager = NetworkManager.getInstance();
		networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_TEST_RUN);
		appendUiLog("Test Run Mode start");
		PlaybackManager.getInstance().play();
		
	}

	@Override
	public void stop() {
		appendUiLog("Test Run Mode stop");
		PlaybackManager.getInstance().stop();
	}

}
