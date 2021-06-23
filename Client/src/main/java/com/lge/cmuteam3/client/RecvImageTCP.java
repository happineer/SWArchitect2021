package com.lge.cmuteam3.client;

import java.io.IOException;
import java.util.Scanner;

public class RecvImageTCP {	

	/**
	 * @param args
	 */
	public static void main(String[] args) throws IOException {
		
		int port = 5000;
		int bufferSize = 10;
		int delay = 80;

		// UI에서 입력 받는 것이 어떨까요
//	    Scanner scanner = new Scanner(System.in);
//		try {
//			System.out.print("Port (Default : 5000) : ");
//			port = Integer.parseInt(scanner.nextLine().trim());
//		} catch(NumberFormatException e) {}
//
//		try {
//			System.out.print("BufferSize (Default : 10) : ");
//			bufferSize = Integer.parseInt(scanner.nextLine().trim());
//		} catch(NumberFormatException e) {}
//
//		try {
//			System.out.print("Delay (Default : 80) : ");
//			delay = Integer.parseInt(scanner.nextLine().trim());
//		} catch(NumberFormatException e) {}
//
//		scanner.close();

		Receiver receiver = new Receiver("192.168.0.229", port, bufferSize);
		Player player = new Player(receiver, delay);
		receiver.start();
		player.start();
	}
}
