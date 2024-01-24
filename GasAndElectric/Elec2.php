<?php
$db = new SQLite3('/mnt/Seagate_4TB_3/root/Software/Hardware/Arduino/arduino-1.8.5/portable/sketchbook/NTPandWebServer/Datalogging.db');
$query = "INSERT INTO sensor_data (ElecReading) VALUES( ".$argv[1]." )";
$db->exec($query);
$db->close();
?>

