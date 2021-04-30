<p>Price is changed to $<?php echo $_GET['value']; ?></p>

<button><a href = "http://ee3070grp2.rf.gd/">Change Price</a></button>

<?php

$value = $_GET['value'];

//API Url
$url = 'https://api.thingspeak.com/update.json';

//Initiate cURL.
$ch = curl_init($url);

//The JSON data.
$jsonData = array(
    'api_key' => 'VNL731QSD3LJL3CQ',
    'field1' => $value,
);

//Encode the array into JSON.
$jsonDataEncoded = json_encode($jsonData);

//Tell cURL that we want to send a POST request.
curl_setopt ($ch, CURLOPT_POST, TRUE);

curl_setopt ($ch, CURLOPT_RETURNTRANSFER, TRUE);

//Attach our encoded JSON string to the POST fields.
curl_setopt($ch, CURLOPT_POSTFIELDS, $jsonDataEncoded);

//Set the content type to application/json
curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json')); 
//Execute the request
$result = curl_exec($ch);

curl_close($ch);

?>

