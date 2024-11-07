class Main inherits IO{
    main(): Object{
        let first: String <- "Hello",
            second: String <- "World!",
            newline: String <- "\n"
        in
            out_string(first.concat(second).concat(newline))
    };
};