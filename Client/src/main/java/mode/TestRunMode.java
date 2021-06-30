package mode;

import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

import java.net.Socket;

public class TestRunMode extends BaseMode {

	private PlaybackManager playbackManager;

	public TestRunMode(UiController uiController) {
		super(uiController);

		playbackManager = new PlaybackManager(uiController);
	}

	@Override
	public String getModeName() {
		return "Test Run Mode";
	}
	
	@Override
	public void start() {
		// TODO : player 는 소켓을 맺어서 inputStream 대기해 놓는 것 까지는 완성해 놓고
		//이 부분에서는 NANO 에 명령어를 전달하는 방식으로 변경
		NetworkManager networkManager = NetworkManager.getInstance();
		Socket socket = networkManager.getNanoSocket();
		if (socket == null) {
			appendUiLog("Test Run Mode failed.");
			return;
		}

		appendUiLog("Test Run Mode start");
		playbackManager.play(socket);
	}

	@Override
	public void stop() {
		appendUiLog("Test Run Mode stop");
		playbackManager.stop();
	}

}
