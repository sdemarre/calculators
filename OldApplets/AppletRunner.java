public class AppletRunner {

    public static void main(String[] args) {
        // Create an instance of the applet
        quad applet = new quad();

        // Initialize the applet (this is typically done by the applet container)
        applet.init();
	
        // Call the startCalc method and capture its result
        String result = applet.doIt();

        // Display the result
        System.out.println("Output from startCalc: " + result);
    }
}
