<?php
$db = new SQLite3('/var/www/html/GasAndElectric/Datalogging.db');
$query = 'INSERT INTO sensor_data (ElecReading) VALUES( ' . htmlspecialchars($_GET["ElecReading"]) . ' )';
$db->exec($query);
$db->close();
?>

