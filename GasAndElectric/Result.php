<?php
$db = new SQLite3('/var/www/html/GasAndElectric/Datalogging.db');
$results = $db->query('select * from sensor_data');
while ($row = $results->fetchArray()) {
print "<pre>";
    var_dump($row);
print "</pre>";
}
$db->close();
?>



