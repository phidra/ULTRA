function zeropad(integer) {
    return integer >= 10 ? `${integer}` : `0${integer}`;
}

export function timestring_to_seconds(timestring) {
    // INPUT  = '13:41:12'
    // OUTPUT = 49272  (= 13*3600 + 41*60 + 12)
    const [hours_str, minutes_str, seconds_str] = timestring.split(":");
    const hours_int = parseInt(hours_str, 10);
    const minutes_int = parseInt(minutes_str, 10);
    const seconds_int = parseInt(seconds_str, 10);
    if (isNaN(hours_int) || isNaN(minutes_int || isNaN(seconds_int))) { throw "unable to parse departure_time"; }
    return hours_int * 3600 + minutes_int * 60 + seconds_int;
}

export function seconds_to_timestring(seconds) {
    // INPUT  = 49272  (= 13*3600 + 41*60 + 12)
    // OUTPUT = '13:41:12'
    const nb_hours = Math.trunc(seconds / 3600);
    const remainder = seconds % 3600;
    const nb_minutes = Math.trunc(remainder / 60);
    const nb_seconds = remainder % 60;
    return `${zeropad(nb_hours)}:${zeropad(nb_minutes)}:${zeropad(nb_seconds)}`;
}

export function seconds_to_duration(seconds) {
    // INPUT  = 49272  (= 13*3600 + 41*60 + 12)
    // OUTPUT = '13h41m12s'
    // INPUT  = 2472
    // OUTPUT = "41m12s"
    const nb_hours = Math.trunc(seconds / 3600);
    const remainder = seconds % 3600;
    const nb_minutes = Math.trunc(remainder / 60);
    const nb_seconds = remainder % 60;
    if (seconds <= 3599) {
        return `${zeropad(nb_minutes)}m${zeropad(nb_seconds)}s`;
    }
    else {
        return `${zeropad(nb_hours)}h${zeropad(nb_minutes)}m${zeropad(nb_seconds)}s`;
    }
}

export function get_next_monday_with_this_time(time_in_seconds, base=null) {
    if (base === null) { base = new Date(); }  // by default, starting from now

    let result = new Date(base);  // now
    const MONDAY = 1;

    // if we already are monday, we want the monday in 7 days :
    result.setDate(result.getDate() + 1);

    while (result.getDay() != MONDAY) {
          result.setDate(result.getDate() + 1);
    }
    //  49260 = 13*3600 + 41*60 -> '13:41'
    const nb_hours = Math.trunc(time_in_seconds / 3600);
    const hour_remainder = time_in_seconds % 3600;
    const nb_minutes = Math.trunc(hour_remainder / 60);
    const nb_seconds = hour_remainder % 60;

    result.setHours(nb_hours);
    result.setMinutes(nb_minutes);
    result.setSeconds(nb_seconds);

    return result;
}

export function format_for_routemm(datetime) {
    const year = datetime.getFullYear();
    const month = datetime.getMonth() + 1;  // month starts at 0
    const day = datetime.getDate();  // en voilà une méthode bien nommée..... grmbl
    const hours = datetime.getHours();
    const minutes = datetime.getMinutes();
    return `${year}${zeropad(month)}${zeropad(day)}T${zeropad(hours)}${zeropad(minutes)}`;
}

export function format_for_navitia(datetime) {
    const year = datetime.getFullYear();
    const month = datetime.getMonth() + 1;  // month starts at 0
    const day = datetime.getDate();  // en voilà une méthode bien nommée..... grmbl
    const hours = datetime.getHours();
    const minutes = datetime.getMinutes();
    const seconds = datetime.getSeconds();
    return `${year}${zeropad(month)}${zeropad(day)}T${zeropad(hours)}${zeropad(minutes)}${zeropad(seconds)}`;
}

