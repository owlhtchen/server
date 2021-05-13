
btn = document.querySelector("#btn");
square = document.querySelector("#square");
btn.onclick = () => {
    console.log("hi ", square.style);
    if("red" == square.style.backgroundColor) {
        square.style.backgroundColor = "BlueViolet";
    } else {
        square.style.backgroundColor = "red";
    }
}