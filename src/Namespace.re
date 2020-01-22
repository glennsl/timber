module Constants = {
  let colors = [|
    // `Black
    `Blue,
    `Cyan,
    `Green,
    `Magenta,
    `Red,
    //`White,
    `Yellow,
    `Hi(`Black),
    `Hi(`Blue),
    `Hi(`Cyan),
    `Hi(`Green),
    `Hi(`Magenta),
    `Hi(`Red),
    `Hi(`White),
    `Hi(`Yellow),
  |];
};

let pickColor = namespace => {
  let index = Hashtbl.hash(namespace) mod Array.length(Constants.colors);
  Constants.colors[index];
};

let pp = ppf => Format.pp_print_string(ppf);
let tag = Logs.Tag.def("namespace", pp);

let includes = ref([]);
let excludes = ref([]);

let isEnabled = namespace => {
  let test = re => Re.execp(re, namespace);
  let included = includes^ == [] || List.exists(test, includes^);
  let excluded = List.exists(test, excludes^);

  included && !excluded;
};

let setFilter = filter => {
  let filters = filter |> String.split_on_char(',') |> List.map(String.trim);

  let (incs, excs) =
    filters
    |> List.fold_left(
         ((includes, excludes), filter) =>
           if (filter == "") {
             (includes, excludes);
           } else if (filter.[0] == '-') {
             let filter =
               String.sub(filter, 1, String.length(filter) - 2)
               |> Re.Glob.glob
               |> Re.compile;

             (includes, [filter, ...excludes]);
           } else {
             let filter = filter |> Re.Glob.glob |> Re.compile;

             ([filter, ...includes], excludes);
           },
         ([], []),
       );

  includes := incs;
  excludes := excs;
};
