<?php

function json_response( $message = null, $code = 200 ) {
	header_remove();
	http_response_code( $code );
	header( "Cache-Control: no-transform,public,max-age=300,s-maxage=900" );
	// treat this as json
	header( 'Content-Type: application/json' );
	$status = array(
		200 => '200 OK',
		400 => '400 Bad Request',
		403 => 'Authorization Required',
		422 => 'Unprocessable Entity',
		500 => '500 Internal Server Error'
	);

	header( 'Status: ' . $status[ $code ] );

	// return the encoded json
	return json_encode( array(
		'status'  => $code < 300, // success or not?
		'message' => $message
	) );


}


$url     = 'https://api.darksky.net/forecast/f86984e69ad8d1c3e32b0fe5d5bddd15/46.4826075,30.7480004?exclude=hourly,flags,alerts&units=ca';
$content = file_get_contents( $url );
$json    = json_decode( $content, true );




$date = new DateTime( null, new DateTimeZone( 'Europe/Kiev' ) );

$response = array(

	'now'      => array(
		'temp' => round( $json['currently']['temperature'] ),
		'icon' => $json['currently']['icon'],
	),
	'today'    => array(
		'temp_max' => round( $json['daily']['data'][0]['temperatureMax'] ),
		'temp_min' => round( $json['daily']['data'][0]['temperatureMin'] ),
		'icon'     => $json['daily']['data'][0]['icon'],
		'rain'     => round($json['daily']['data'][0]['precipProbability'] * 10) * 10 //60%
	),
	'tomorrow' => array(
		'temp_max' => round( $json['daily']['data'][1]['temperatureMax'] ),
		'temp_min' => round( $json['daily']['data'][1]['temperatureMin'] ),
		'icon'     => $json['daily']['data'][1]['icon'],
		'rain'     => round($json['daily']['data'][1]['precipProbability'] * 10) * 10
	),

	'time' => $date->format( 'H:i' ),
	'dt'   => $date->getTimestamp()
);


echo json_response( $response );
die();






