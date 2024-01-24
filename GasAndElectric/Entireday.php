<?php

    $db = new SQLite3('/var/www/html/GasAndElectric/Datalogging.db',SQLITE3_OPEN_READONLY);

   if (!$db) die ($error);

   /*$results = $db->query('SELECT * FROM readings');*/

    $statement = $db->prepare("select strftime('%Y-%m-%dT%H%M', datetime(sqlitetimestamp,'localtime')) as ReadingDate, count(ElecReading) as ElecUsage, count(GasReading) as GasUsage from sensor_data where  strftime('%Y-%m-%dT', datetime(sqlitetimestamp,'localtime')) = strftime('%Y-%m-%dT', datetime('now','localtime')) GROUP by strftime('%Y-%m-%dT%H%M', datetime(sqlitetimestamp,'localtime'))");
    $results = $statement->execute();
    if (!$results) die("Cannot execute statement.");
    $i = 0;
    while ($row = $results->fetchArray()) 
    {
        /*var_dump($row);*/
        /*echo $row['r_temp']  . " : " . $row['r_watts'];
        echo "<br>";*/
        $ReadingDate[$i] = $row['ReadingDate'];
        $Elec[$i] = $row['ElecUsage'];
        $Gas[$i] = $row['GasUsage'];
        $i=$i+1;
    }
    /*$maxwatts = 60;*/
    $maxElec = max($Elec);
    $maxGas = max($Gas);

    /*Set parameters for width & height of chart*/
    $width = 1440;
    $height = 500;
    $tickerlength = 1;
    $scalefactorElec = $height / $maxElec;
    $scalefactorGas = $height / $maxGas;
    $totalheight= $height;
    $xticker = 1;

echo 'MaxElec is "'.$maxElec.'"<br>
';
echo 'MaxGas is "'.$maxGas.'"<br>
';
echo 'Record Count is "'.count($Elec).'"<br>
';

    echo '<svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="'.$width.'" height="'.$totalheight.'" >
<line x1="0" y1="'.$height.'" x2="'.$width.'" y2="'.$height.'" style="stroke:black;stroke-width:2"/>
<line x1="0" y1="0"           x2="0"          y2="'.$height.'" style="stroke:black;stroke-width:2"/>
';
$polystringElec = '';

    for ($j = 0; $j < count($Elec); $j++) {
        $x = $j*$xticker;
        $tickerheight = $height+$tickerlength;
        echo '<!-- <line x1="'.$x.'" y1="'.$height.'" x2="'.$x.'" y2="'.$tickerheight.'" style="stroke:black;stroke-width:1"/> -->
';

        echo '<!-- x is "'.$x.'" -->
';
        echo '<!-- Height is "'.$height.'" -->
';
        echo '<!-- tickerheight is "'.$tickerheight.'" -->
';




        echo '<!-- ReadingDate is "'.$ReadingDate[$j].'" -->
';
        echo '<!-- Elec is "'.$Elec[$j].'" -->
';
        $yGas = $height - $Gas[$j]*$scalefactorGas;
        $polystringGas =  $polystringGas." ".$x.",".$yGas;
        echo '<!-- polystringGas is "'.$polystringGas.'" -->
';
        $yElec = $height - $Elec[$j]*$scalefactorElec;
        $polystringElec = $polystringElec." ".$x.",".$yElec;
        echo '<!-- polystringElec is "'.$polystringElec.'" -->
';


    }
    echo '<polyline points="'.$polystringGas.'" style="fill:none;stroke:red;stroke-width:4"/>';
    echo '<polyline points="'.$polystringElec.'" style="fill:none;stroke:green;stroke-width:1"/>';    
    echo '</svg>';
/* echo '<p>ReadingDate is "'.$maxElec.'" </p><br>';*/


?>

