import Cookies from 'js-cookie';

export function genCSRFOptions () {
    let csrf_token = Cookies.get("csrftoken");
    let options = {};
    if(csrf_token) {
        options.headers = {
            'X-CSRFToken': csrf_token
        }
    }
    return options;
}
