if (match(PLUS, MINUS, STAR, SLASH,
          BANG_EQUAL, EQUAL_EQUAL,
          GREATER, GREATER_EQUAL,
          LESS, LESS_EQUAL)){

    error(previous(), "Missing left hand operand.");

    term();

    return null;
}