<?php

class IcingaConnection
{
	private $_streamContext;
	private $_stream;
	private $_subscriptions;
	private $_publications;

	public function __construct()
	{
		$this->_subscriptions = array();
		$this->_publications = array();
	}

	public function Connect($host, $port, $cafile, $cert)
	{
		$this->_streamContext = stream_context_create();
		stream_context_set_option($this->_streamContext, 'ssl', 'cafile', $cafile);
		stream_context_set_option($this->_streamContext, 'ssl', 'local_cert', $cert);

		$this->_stream = stream_socket_client('ssl://' . $host . ':' . $port, $error, $errorString, 60, STREAM_CLIENT_CONNECT, $this->_streamContext);

		if ($this->_stream === false)
			return false;

		$this->Publish('discovery::RegisterComponent');
		$this->Publish('discovery::Welcome');

		$this->Subscribe('discovery::NewComponent');
		$this->Subscribe('discovery::RegisterComponent');
		$this->Subscribe('discovery::Welcome');

		$params = array(
			'publications' => $this->_publications,
			'subscriptions' => $this->_subscriptions
		);
		$this->SendMessage('discovery::RegisterComponent', $params);
		$this->SendMessage('discovery::Welcome');
	}

	public function Subscribe($topic)
	{
		$this->_subscriptions[] = $topic;
	}

	public function Publish($topic)
	{
		$this->_publications[] = $topic;
	}

	public function SendMessage($topic, $params = array())
	{
		$message = array('jsonrpc' => '2.0', 'method' => $topic, 'params' => $params);
		$messageStr = json_encode($message, JSON_FORCE_OBJECT);

		fprintf($this->_stream, '%d:%s,', strlen($messageStr), $messageStr);
		fflush($this->_stream);
	}

	public function ReadMessage()
	{
		$lenStr = '';

		/* read netstring length (up until the first :) */
		while (($chr = fread($this->_stream, 1)) != ':') {
			if ($chr === false)
				return false;

			$lenStr .= $chr;
		}

		/* read data */
		$messageStr = fread($this->_stream, (int)$lenStr);

		if ($messageStr === false)
			return false;

		/* read trailing comma */
		$comma = fread($this->_stream, 1);

		if ($comma != ',')
			return false;

		return json_decode($messageStr, true);
	}
}

$icinga = new IcingaConnection();
$icinga->Subscribe('delegation::ServiceStatus');
if ($icinga->Connect('localhost', 7777, 'ca.crt', 'icinga-c3.pem') === false)
	return;

while (($message = $icinga->ReadMessage()) !== false) {
	if ($message['method'] != 'delegation::ServiceStatus')
		continue;

	$params = $message['params'];

	$now = time();

	$next = $params['next_check'] - $now;

	$states = array('ok', 'warning', 'critical', 'uncheckable', 'unknown');
	$types = array('hard', 'soft');

	echo 'Service: ' . $params['service'] .
	     ', State: ' . $states[$params['state']] .
	     ', State Type: ' . $types[$params['state_type']] .
	     ', Attempt: ' . $params['current_attempt'] .
	     ' (took ' . ($params['result']['schedule_end'] - $params['result']['schedule_start']) . ' seconds' .
	     ', next check in ' . $next . ' seconds)' .
	     "\n";
}

?>
