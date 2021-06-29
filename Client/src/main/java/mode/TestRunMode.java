package mode;

import com.lge.cmuteam3.client.PlaybackManager;
import com.lge.cmuteam3.client.ui.BaseFrame;

public class TestRunMode implements Mode {

	@Override
	public String getModeName() {
		return "Test Run Mode";
	}
	
	@Override
	public void start() {
		// TODO : player 는 소켓을 맺어서 inputStream 대기해 놓는 것 까지는 완성해 놓고
		//이 부분에서는 NANO 에 명령어를 전달하는 방식으로 변경
		BaseFrame.getInstance().appendLog("Test Run Mode start");
        PlaybackManager.getInstance().play();
		
	}

	@Override
	public void stop() {
		BaseFrame.getInstance().appendLog("Test Run Mode stop");
		
	}

}
