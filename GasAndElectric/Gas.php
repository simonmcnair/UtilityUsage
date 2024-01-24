<?php
$db = new SQLite3('/var/www/html/GasAndElectric/Datalogging.db');
$query = 'INSERT INTO sensor_data (GasReading) VALUES( ' . htmlspecialchars($_GET["GasReading"]) . ' )';
$db->exec($query);
$db->close();
?>

