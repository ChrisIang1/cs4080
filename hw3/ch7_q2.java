@Override
public Object visitBinaryExpr(Expr.Binary expr) {
    Object left = evaluate(expr.left);
    Object right = evaluate(expr.right); // [left]

    switch (expr.operator.type) {
//> binary-equality
        case BANG_EQUAL: return !isEqual(left, right);
        case EQUAL_EQUAL: return isEqual(left, right);
//< binary-equality
//> binary-comparison
        case GREATER:
//> check-greater-operand
            checkNumberOperands(expr.operator, left, right);
//< check-greater-operand
            return (double)left > (double)right;
        case GREATER_EQUAL:
//> check-greater-equal-operand
            checkNumberOperands(expr.operator, left, right);
//< check-greater-equal-operand
            return (double)left >= (double)right;
        case LESS:
//> check-less-operand
            checkNumberOperands(expr.operator, left, right);
//< check-less-operand
            return (double)left < (double)right;
        case LESS_EQUAL:
//> check-less-equal-operand
            checkNumberOperands(expr.operator, left, right);
//< check-less-equal-operand
            return (double)left <= (double)right;
//< binary-comparison
        case MINUS:
//> check-minus-operand
            checkNumberOperands(expr.operator, left, right);
//< check-minus-operand
            return (double)left - (double)right;
//> binary-plus
        case PLUS:
            if (left instanceof Double && right instanceof Double) {
                return (double)left + (double)right;
            } // [plus]

            if (left instanceof String || right instanceof String) {
                return stringify(left) + stringify(right);
            }

/* Evaluating Expressions binary-plus < Evaluating Expressions string-wrong-type
        break;
*/
//> string-wrong-type
            throw new RuntimeError(expr.operator,
                    "Operands must be two numbers or two strings.");
//< string-wrong-type
//< binary-plus
        case SLASH:
//> check-slash-operand
            checkNumberOperands(expr.operator, left, right);
//< check-slash-operand
            return (double)left / (double)right;
        case STAR:
//> check-star-operand
            checkNumberOperands(expr.operator, left, right);
//< check-star-operand
            return (double)left * (double)right;
    }

    // Unreachable.
    return null;
}