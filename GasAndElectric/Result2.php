<?php
$db = new SQLite3('/var/www/html/GasAndElectric/Datalogging.db');

$results = $db->query("select strftime('%Y-%m-%dT%H%M', sqlitetimestamp) as ReadingDate, count(ElecReading) as Usage from sensor_data where  strftime('%Y-%m-%dT', sqlitetimestamp) = strftime('%Y-%m-%dT', 'now') GROUP by strftime('%Y-%m-%dT%H%M', sqlitetimestamp)");

while ($row = $results->fetchArray()) {
print "<pre>";
    var_dump($row);
print "</pre>";
}
$db->close();
?>



