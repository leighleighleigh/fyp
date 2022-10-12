<?php

/*
# Sensors In Green Infrastructure
    A Monash University
    Engineering Final Year Project

Date: October 12th 2022
Author: Leigh Oliver
Supervisor: Dr. Brandon Winfrey

# Description
    This script lives at http://bosl.com.au/IoT/wsudwatch/FYP_SGI/log.php
    and processes incoming HTTP POST requests from BoSL board devices used in this project.
    The devices send a JSON-format message, each with a unique 'device_name' key.
    This key is used to log the entire JSON message to a file of the same name, 
    and also is used to generate a CSV file equivalent for easy processing.
*/


/*
Handles the POST request, turning the JSON payload into a PHP object.
Validates that the 'device_name' key exists.
Returns the object variable for further processing.
*/
function parseJSONPayload()
{
    // Takes raw data from the request
    $json = file_get_contents('php://input');
    // Converts it into a PHP object
    $data = json_decode($json,true);

    // SCHEMA VALIDATION
    if ( ! isset($data['device_name']) )
    {
        http_response_code(400);
        exit("device_name key is required");
    }

    // ADD DATE-CREATED FIELD
    // This is more robust than relying on the modem-reported value
    date_default_timezone_set("Australi/Melbourne");
    $time_in_Detroit = date('Y-m-d H:i:s', time());
    $utc_time = gmdate("Y-m-d  H:i:s");
    $data["date_created_utc"] = $utc_time;

    // Return payload object
    return $data;
}

/* 
Resolves the filepath for the request, given a device_name key
*/
function getFilePath($data, $filetype = 'json'){
    $baseurl = "./";
    // Extract filename from JSON request, in the 'device_name' key
    $boslname = $data['device_name'];
    $logfilepath = $baseurl . $boslname . "." . $filetype;
    return $logfilepath;
}

/*
Given a variable object input, writes it out as a JSON-encoded string 
to the filename <device_name>.json.
*/
function writeJSONtoFile($data){
    $logfilepath = getFilePath($data);
    // Turn the PHP request back into a JSON string for logging
    $json_string = json_encode($data);
    // Append the JSON line to file, creating a new file if needed.
    $file_handle = fopen($logfilepath, 'a+');
    // Lines follow 
    fwrite($file_handle, $json_string . "\r\n");
    fclose($file_handle);
}

/* 
Given an object argument, extracts all the keys defined in the object (INCLUDING null keys),
and generates a CSV file entry - plus the required headers if the file doesn't exist.
*/
function writeJSONtoCSVFile($data)
{
    $csvdata = array();
    // Convert to array for fputcsv
    foreach ($data as $key => $val) {
        // If key was not set in data (is null), replace with 'null' string.
        if ( ! isset($val) )
        {
            $csvdata[$key] = "null";
        }else{
            $csvdata[$key] = $val;
        }
        echo $key."=".$csvdata[$key]."\n";
    }

    // Get file path
    $logfilepath = getFilePath($data, "csv");
    $putheaders = false;

    // If file exists before open, we will write the headers to the file first
    if ( ! file_exists($logfilepath) )
    {
        $putheaders = true;
    }

    // Append the JSON line to file, creating a new file if needed.
    $file_handle = fopen($logfilepath, 'a+');

    // Headers
    if($putheaders){
        fputcsv($file_handle, array_keys($csvdata));
    }

    // Data
    fputcsv($file_handle, $csvdata);
    // fwrite($file_handle, "\r\n");
    fclose($file_handle);
}


// Handle request, resolve logfile path, write JSON, and write CSV!
$data = parseJSONPayload();
writeJSONtoFile($data);
writeJSONtoCSVFile($data);
?>
