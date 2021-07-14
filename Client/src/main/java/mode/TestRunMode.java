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
		super.start();
		NetworkManager networkManager = NetworkManager.getInstance();
		PlaybackManager.getInstance().play(Constants.CONTROL_VALUE_TEST_RUN);
		networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_TEST_RUN);
		
		
	}

	@Override
	public void stop() {
		super.stop();
		PlaybackManager.getInstance().stop();
	}

	@Override
	public RunningButtonMode getRunningButtonMode() {
		return RunningButtonMode.DISABLE_ALL_EXCEPT_CURRENT;
	}
}
