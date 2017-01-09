import java.io.*;
import java.net.*;
import java.util.*;

public class HTTPServer implements Runnable {

	// mini-webserver constants and variables
	private BufferedReader in;
	private PrintStream out;
	private BufferedReader htmlTemplate;
	private int port = 1989;
	private static ServerSocket serverSocket;
	private Socket clientSocket;
	// template filename
	private String template = "login.html";
	// post variable
	private boolean readPost = false;
	// in/out constants/variable for mini-webserver
	private static final String LogInPage = "<div class='box' id='mybox'><form method='POST' style=''><br> Username:<br> <input type='text' name='username'><br> Password:<br> <input type='password' name='password'><br> <input type='submit' value='Log In'></form></div><a class='newpass' href='/setPassword'>Set a new gesture password</a></div>";
	private static final String messageHTML = "<div class='bigboss'><div class='response' id='mymessage'>%</div>";
	private static final String inputGesture = "<div class='box' id='mybox'><div class = 'gesture'>Please input gesture key</div>";
	private static String message = " ";
	private static int toDisplay = 0;
	private String post = "";
	private static final String password = "parola";
	private static final String username = "cezar";
	private static String gesturePassword = "223";
	private int reqType;

	HTTPServer() {
		try {
			serverSocket = new ServerSocket(port);
			System.err.println("Server started on port : " + port);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	HTTPServer(Socket clientSocket) {
		this.clientSocket = clientSocket;
		try {
			in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
			out = new PrintStream(clientSocket.getOutputStream(), true);
			post = "";
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void run() {
		read();
		if(reqType == 3){
			close(); //This is in case favicon is requested
		}
		checkPassword();
		if (toDisplay == 2 && reqType == 0)
			checkGesturePassword();
		if (reqType == 2)
			changeGesturePassword();
		send();
		close();
	}

	public void start() {
		try {
			clientSocket = serverSocket.accept();
			System.err.println("New client connected");
			new Thread(new HTTPServer(clientSocket)).start();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void changeGesturePassword(){
		message = "Connecting to gesture device";
		send();
		close();
		SerialTest Arduino = new SerialTest("COM13");
		Arduino.initialize();
		System.err.println("Gesture device is connected");
		message = "Gesture device connected";
		message = "Input current gesture";
		String getFromArduino;
		boolean okGesture = false;
		while (!okGesture) {
			do {
				getFromArduino = Arduino.readLine();
			} while (!getFromArduino.startsWith("Password:"));
			String[] result = getFromArduino.split(":");
			try {
				System.out.println(result[1]);
				if (result[1].equals(gesturePassword)) {
					message = "Input new Gesture";
					okGesture = true;
					System.out.println("Correct Gesture");
				} else
					message = "Incorrect gesture";
			} catch (ArrayIndexOutOfBoundsException e) {
				message = "Incorrect gesture";
			}
		}
			do {
				getFromArduino = Arduino.readLine();
			} while (!getFromArduino.startsWith("Password:"));
			String[] result = getFromArduino.split(":");
			try {
				System.out.println(result[1]);
				gesturePassword = result[1];
				message = "Success";
			} catch (ArrayIndexOutOfBoundsException e) {
				message = "Incorrect gesture";		
		}
		System.err.println("Connection with gesture device ended");
		Arduino.close();
	}

	public void read() {
		String contentHeader = "Content-Length: ";
		int contentLength = 0;
		String s;
		int c;
		// Go back to homepage if successfully logged in
		try {
			while (!(s = in.readLine()).equals("")) {
			 //System.out.println(s);
				if (s.startsWith("POST /getMessage")) {
					reqType = 1;
					System.out.println("reqType1");
				} else if(s.startsWith("GET /setPassword")){
					reqType = 2;
					System.out.println("reqType2");
				} else if(s.startsWith("GET /favicon.ico")){
					reqType = 3;
					System.out.println("reqType3 (Favicon)");
				} else if (s.startsWith("GET")) {
					reqType = 0;
					System.out.println("reqType0");
				}
				
				if (s.startsWith(contentHeader)) {
					contentLength = Integer.parseInt(s.substring(contentHeader.length()));
					readPost = true;
				}
			}

			if (readPost) {
				for (int i = 0; i < contentLength; i++) {
					c = in.read();
					post += (char) c;
				}
				// System.out.println(post);
				readPost = false;
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void close() {
		System.err.println("Connection with client ended");
		try {
			out.close();
			in.close();
			clientSocket.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	public void checkGesturePassword() {
		message = "Connecting to gesture device";
		send();
		close();
		SerialTest Arduino = new SerialTest("COM13");
		Arduino.initialize();
		System.err.println("Gesture device is connected");
		message = "Gesture device connected";
		String getFromArduino;
		boolean okGesture = false;
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			System.out.println("intrerrupted");
		} 
		while (!okGesture) {
			do {
				getFromArduino = Arduino.readLine();
			} while (!getFromArduino.startsWith("Password:"));
			String[] result = getFromArduino.split(":");
			try {
				System.out.println(result[1]);
				if (result[1].equals(gesturePassword)) {
					toDisplay = 3;
					message = "Welcome!";
					okGesture = true;
					System.out.println("Correct Gesture");
				} else
					message = "Incorrect gesture";
			} catch (ArrayIndexOutOfBoundsException e) {
				message = "Incorrect gesture";
			}
		}
		System.err.println("Connection with gesture device ended");
		Arduino.close();
	}

	public void checkPassword() {
		if (toDisplay == 3 && reqType == 0) {
			toDisplay = 0;
			message = " ";
		}
		boolean okPassword = false;
		boolean okUsername = false;
		// System.out.println("POST:" + post);
		if (!(post.equals(""))) {
			String[] inputs = post.split("=|&");
			try {
				for (int x = 0; x < inputs.length; x++) {
					switch (inputs[x]) {
					case "username": {
						if (inputs[x + 1].equals(username))
							okUsername = true;
						break;
					}
					case "password": {
						if (inputs[x + 1].equals(password))
							okPassword = true;
						break;
					}
					}
				}
			} catch (ArrayIndexOutOfBoundsException e) {
				System.out.println("One of the inputs are null/missing");
			}
			if (okUsername && okPassword) {
				toDisplay = 2;
				message = ""; // Clear message
			} else {
				toDisplay = 1;
				message = "Incorrect password!";
			}
		}
	}

	public void send() {
		try {
			htmlTemplate = new BufferedReader(new FileReader(template));
			String line;
			if (reqType != 1) {
				out.println("HTTP/1.0 200 OK");
				out.println("Content-Type: text/html");
				out.println();

				while ((line = htmlTemplate.readLine()) != null) {
					if (line.startsWith("$")) {
						String tempLine = messageHTML;
						line = tempLine.replaceFirst("%", message);
					}
					if (line.startsWith("%")) {
						switch (toDisplay) {
						case 0:
						case 1: {
							line = LogInPage;
							break;
						}
						case 2: {
							line = inputGesture;
							break;
						}
						case 3: {
							//Here we just send the message "Welcome!"
							break;
						}
						}
					}
					//System.out.println(line);
					out.println(line);
				}
			} else if (reqType == 1) {
				// System.out.println(message);
				out.println(message);
			}

		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public static void main(String[] args) throws Exception {
		HTTPServer server = new HTTPServer();
		while (true) {
			server.start();
		}
	}

}
